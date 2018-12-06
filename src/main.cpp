#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include <spark.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <stdlib.h>
#include <EEPROM.h>
#include <ArduinoJson.h>


// Network stuff, might already be defined by the build tools
#ifdef WLANCONFIG
  #include <WlanConfig.h>
#endif
#ifndef SSID
  #define SSID      WlanConfig::Ssid
#endif
#ifndef PASS
  #define PASS      WlanConfig::Password
#endif
#ifndef NAME
  #define NAME      "NeoXmas"
#endif
#ifndef VERSION
  #define VERSION   NAME " " __DATE__ " " __TIME__
#endif
#ifndef PORT
  #define PORT      80
#endif

#define ONLINE_LED_PIN   D4

// Neopixel stuff
#define PIXEL_PIN        D5
#define NUM_PIXELS       50
#define NEO_CONFIG       (NEO_RGB+NEO_KHZ800)

// Update interval. Increase, if you want to save time for other stuff...
#define INTERVAL_MS       0
// Min and max/2 time for one full animation circle of a led
#define CIRCLE_MS     10000

// Change, if you modify eeprom_t in a backward incompatible way
#define EEPROM_MAGIC     (0xabcd1235)

// EEPROM data
typedef struct {
  uint32_t mode;      // blink/animation mode
  uint32_t msCircle;  // min ms for an animation circle
  uint32_t magic;     // verify eeprom data is ours
} eeprom_t;

// Animation data
typedef struct {
    timedSpark spark;
} animation_t;

// Animation function
typedef uint32_t (*animator_t)(unsigned long t, unsigned pixel);

uint32_t mode;                     // current animation (index to animators[])
uint32_t msCircle;                 // min ms for an animation circle
uint32_t prevMode;                 // previous loop animation

animation_t pixelData[NUM_PIXELS]; // animation data of each pixel
themedSpark themedSparks[NUM_PIXELS];
randomSpark randomSparks[NUM_PIXELS];

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_CONFIG);

ESP8266WebServer web_server(PORT);

ESP8266HTTPUpdateServer esp_updater;


// animation implementations

// simple all white animation
uint32_t all_white(unsigned long t, unsigned pixel) {
  return 0xffffff;
}


// simple all black animation
uint32_t all_black(unsigned long t, unsigned pixel) {
  return 0x000000;
}


// theme spark animation
uint32_t theme_sparks(unsigned long t, unsigned pixel, const baseSpark::color_t colors[], size_t numColors ) {
  themedSpark::color_t color;

  if( prevMode != mode ) {
    themedSpark::setTheme(colors, numColors);
    themedSparks[pixel].reset();
    pixelData[pixel].spark.setSpark(&themedSparks[pixel], msCircle);
  }

  if( pixelData[pixel].spark.get(color) ) {
    uint32_t col = color.r << 16 | color.g << 8 | color.b;
    //Serial.printf("Theme 1 p0 %06x = %02x-%02x-%02x\n", col, color.r, color.g, color.b);
    return col;
  }

  return 0x000000;
}


// theme red-violet-blue spark animation
uint32_t theme_red_violet_blue_sparks(unsigned long t, unsigned pixel) {
  static const baseSpark::color_t colors[] = {
    {0xff, 0, 0},
    {0xff, 0, 0xff},
    {0,    0, 0xff}
  };

  return theme_sparks(t, pixel, colors, sizeof(colors)/sizeof(*colors));
}


// theme red-green-white spark animation
uint32_t theme_red_green_white_sparks(unsigned long t, unsigned pixel) {
  static const baseSpark::color_t colors[] = {
    {0xff,    0,    0},
    {0,    0xff, 0},
    {0xff, 0xff, 0xff}
  };

  return theme_sparks(t, pixel, colors, sizeof(colors)/sizeof(*colors));
}


// theme gold-blue-cyan-green spark animation
uint32_t theme_gold_blue_cyan_green_sparks(unsigned long t, unsigned pixel) {
  static const baseSpark::color_t colors[] = {
    {0xcc, 0x9b, 0x29},
    {0,    0,    0xff},
    {0,    0xff, 0xff},
    {0,    0xff, 0}
  };

  return theme_sparks(t, pixel, colors, sizeof(colors)/sizeof(*colors));
}


// theme blue-green-cyan spark animation
uint32_t theme_green_blue_cyan_sparks(unsigned long t, unsigned pixel) {
  static const baseSpark::color_t colors[] = {
    {0, 0,    0xff},
    {0, 0xff, 0},
    {0, 0xff, 0xff}
  };

  return theme_sparks(t, pixel, colors, sizeof(colors)/sizeof(*colors));
}


// theme white spark animation
uint32_t theme_white_sparks(unsigned long t, unsigned pixel) {
  static const baseSpark::color_t colors[] = {
    {0, 0, 0}
  };

  return theme_sparks(t, pixel, colors, sizeof(colors)/sizeof(*colors));
}


// random spark animation
uint32_t random_sparks(unsigned long t, unsigned pixel) {
  randomSpark::color_t color;
  if( prevMode != mode ) {
    randomSparks[pixel].reset();
    pixelData[pixel].spark.setSpark(&randomSparks[pixel], msCircle);
  }
  if( pixelData[pixel].spark.get(color) ) {
    uint32_t col = color.r << 16 | color.g << 8 | color.b;
    //Serial.printf("Random p0 %06x = %02x-%02x-%02x\n", col, color.r, color.g, color.b);
    return col;
  }

  return 0x000000;
}


// list of animation functions defined above
animator_t animators[] = {
  // first entry is default (make it a nice one...)
  theme_red_violet_blue_sparks,
  theme_red_green_white_sparks,
  theme_gold_blue_cyan_green_sparks,
  theme_green_blue_cyan_sparks,
  random_sparks,
  theme_white_sparks,
  all_white,
  all_black
};

animator_t &animator = animators[0];   // current animation


// Builtin default settings, used if Eeprom is erased or invalid
void setupDefaults() {
  mode = 0;             // first mode
  prevMode = mode - 1;  // different from mode forces mode init
  msCircle = CIRCLE_MS; // default min animation circle time
}

// Erase saved settings
void clearEeprom() {
  eeprom_t data {0};
  EEPROM.put(0, data);
  EEPROM.commit();
}


// Save current settings permanently
void setEeprom() {
  eeprom_t data { mode, msCircle, EEPROM_MAGIC };
  EEPROM.put(0, data);
  EEPROM.commit();
}


// Read previously saved settings
void getEeprom() {
  eeprom_t data;
  EEPROM.get(0, data);
  if( data.magic == EEPROM_MAGIC ) {
    mode = data.mode;
    msCircle = data.msCircle;
  }
}


// Initiate connection to Wifi but dont wait for it to be established
void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(NAME);
  WiFi.begin(SSID, PASS);
  pinMode(ONLINE_LED_PIN, OUTPUT);
  digitalWrite(ONLINE_LED_PIN, HIGH);
}


// Call this after mode has been changed to setup new animation
void setupAnimation() {
  animator = animators[mode < sizeof(animators)/sizeof(*animators) ? mode : 0];
}


// default html menu page
void send_menu() {
  static const char form[] = "<!doctype html>\n"
    "<html lang=\"en\">\n"
    "  <head>\n"
    "    <meta charset=\"utf-8\">\n"
    "    <meta name=\"description\" content=\"NeoXmas Web Remote Control\">\n"
    "    <meta name=\"keywords\" content=\"NeoXmas, neopixel, remote, meta\">\n"
    // "    <link rel=\"stylesheet\" href=\"css\">\n"
    "    <title>NeoXmas Web Remote Control</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>NeoXmas Web Remote Control</h1>\n"
    "    <p>Control the Xmas Neopixel Strip Animations</p>\n"
    "    <form action=\"cfg\">\n"
    "      <label for=\"mode\">Mode:\n"
    "        <select name=\"mode\">\n"
    "          <option %svalue=\"0\">Sparks red-violet-blue</option>\n"
    "          <option %svalue=\"1\">Sparks red-green</option>\n"
    "          <option %svalue=\"2\">Sparks yellow-blue</option>\n"
    "          <option %svalue=\"3\">Sparks green-cyan-blue</option>\n"
    "          <option %svalue=\"4\">Sparks random</option>\n"
    "          <option %svalue=\"5\">Sparks white</option>\n"
    "          <option %svalue=\"6\">On</option>\n"
    "          <option %svalue=\"7\">Off</option>\n"
    "        </select>\n"
    "      </label></p>\n"
    "      <label for=\"circle\">Animation Speed:\n"
    "        <select name=\"circle\">\n"
    "          <option %svalue=\"100\">Super fast</option>\n"
    "          <option %svalue=\"500\">Very fast</option>\n"
    "          <option %svalue=\"1000\">Fast</option>\n"
    "          <option %svalue=\"4000\">Normal</option>\n"
    "          <option %svalue=\"10000\">Slow</option>\n"
    "          <option %svalue=\"20000\">Very slow</option>\n"
    "          <option %svalue=\"60000\">Super slow</option>\n"
    "        </select>\n"
    "      </label></p>\n"
    "      <button>Configure</button>\n"
    "    </form></p>\n"
    "    <form action=\"/reset\">\n"
    "      <button>Reset</button>\n"
    "    </form></p>\n"
    "    <form action=\"/clear\">\n"
    "      <button>Clear</button>\n"
    "    </form></p>\n"
    "    <form action=\"/version\">\n"
    "      <button>Version</button>\n"
    "    </form></p>\n"
    "  </body>\n"
    "</html>\n";
  static const char sel[] = "selected ";
  static char page[sizeof(form)+3*10];

  snprintf(page, sizeof(page), form, mode==0?sel:"", mode==1?sel:"",
    mode==2?sel:"", mode==3?sel:"", mode==4?sel:"",
    mode==5?sel:"", mode==6?sel:"", mode==7?sel:"",
    msCircle==100?sel:"", msCircle==500?sel:"", msCircle==1000?sel:"",
    msCircle==4000?sel:"", msCircle==10000?sel:"", msCircle==20000?sel:"",
    msCircle==60000?sel:""
  );

  web_server.send(200, "text/html", page);
}


// Define web pages for update, reset or for configuring parameters
void webserverSetup() {

  // Call this page to see the ESPs firmware version
  web_server.on("/version", []() {
    web_server.send(200, "text/plain", "ok: " VERSION "\n");
  });

  // Call this page to reset the ESP
  web_server.on("/reset", []() {
    web_server.send(200, "text/plain", "ok: reset\n");
    delay(200);
    ESP.restart();
  });

  // Call this page to clear the saved settings (animation mode)
  web_server.on("/clear", []() {
    clearEeprom();
    setupDefaults();
    setupAnimation();
    web_server.send(200, "text/plain", "ok: cleared\n");
  });

  // This page configures all settings (/cfg?name=value{&name=value...})
  web_server.on("/cfg", []() {
    typedef struct arg {
      const char* name; // Parameter name used in the URI
      const char  type; // Parameter type: (f)loat or (u)nsigned int
      void       *pval; // Pointer to the variable receiving the changed value
    } arg_t;

    // Supported parameters
    arg_t args[] = {
      { "mode",   'u', &mode     },
      { "circle", 'u', &msCircle } };

    bool ok = true;    // So far all processed URI parameters were ok
    int processed = 0; // Count processed URI parameters

    if( web_server.args() == 0 ) {
      DynamicJsonBuffer jsonBuffer(200);
      JsonObject& root = jsonBuffer.createObject();
      root["version"] = VERSION;
      JsonObject& cfg = root.createNestedObject("cfg");
      cfg["mode"] = mode;
      cfg["circle"] = msCircle;
      // cfg["l"] = l;
      // JsonObject& color = cfg.createNestedObject("colors");
      // color["p"] = pd_color;
      String msg;
      root.printTo(msg);
      web_server.send(200, "application/json", msg);
    }
    else {
      for( size_t i=0; i<sizeof(args)/sizeof(*args); i++ ) {
        if( web_server.hasArg(args[i].name) ) {
          switch( args[i].type ) {
            case 'f':
              *(float *)(args[i].pval) = web_server.arg(args[i].name).toFloat();
              processed++;
              break;
            case 'u':
              *(uint32_t *)(args[i].pval) = strtoul(web_server.arg(args[i].name).c_str(), NULL, 0);
              processed++;
              break;
            default:
              ok=false;
          }
        }
      }

      // Use and save new values only if all parameters were processed and ok
      if( ok && processed == web_server.args() ) {
        setupAnimation();
        setEeprom();
        send_menu();
      }
      else {
        // Dont use the parameters. Give a hint on what went wrong
        String msg("error: use ");
        String sep("");
        for( size_t i=0; i<sizeof(args)/sizeof(*args); i++ ) {
          msg += sep + args[i].name;
          sep=",";
        }
        web_server.send(400, "text/plain", msg + "\n");
      }
    }
  });

  // This page configures all settings (/cfg?name=value{&name=value...})
  web_server.on("/", []() {
    send_menu();
  });

  // Catch all page, gives a hint on valid URLs
  web_server.onNotFound([]() {
    web_server.send(404, "text/plain", "error: use "
      "/cfg?parm=value[&parm=value...] /reset, /clear, /version or "
      "post image to /update\n");
  });

  web_server.begin();

  MDNS.addService("http", "tcp", PORT);
}


// Handle online web updater, initialize it after Wifi connection is established
void updaterHandle() {
  static bool updater_needs_setup = true;

  if( WiFi.status() == WL_CONNECTED ) {
    if( updater_needs_setup ) {
      // Init once after connection is (re)established
      digitalWrite(ONLINE_LED_PIN, LOW);
      Serial.printf("WLAN '%s' connected with IP ", SSID);
      Serial.println(WiFi.localIP());

      MDNS.begin(NAME);

      esp_updater.setup(&web_server);
      webserverSetup();

      Serial.println("Update with curl -F 'image=@firmware.bin' " NAME ".local/update");

      updater_needs_setup = false;
    }
    web_server.handleClient();
  }
  else {
    if( ! updater_needs_setup ) {
      // Cleanup once after connection is lost
      digitalWrite(ONLINE_LED_PIN, HIGH);
      updater_needs_setup = true;
    }
  }
}


// Set pixels according to animation data
void set_animation_pixels( unsigned long t ) {
  unsigned long pixel_color;

  // Recalculate and set colors of all pixels
  for( unsigned pixel=0; pixel<NUM_PIXELS; pixel++ ) {
    pixel_color = (*animator)(t, pixel);
    // Serial.printf("set_animation_pixels t=%4ld, c=%06lx\n", t, pixel_color);
    pixels.setPixelColor(pixel, pixel_color);
  }

  prevMode = mode;
}


// Setup on boot
void setup() {
  Serial.begin(115200);

  // Initiate network connection (but dont wait for it)
  wifiSetup();

  // Init the neopixels
  pixels.begin();
  pixels.setBrightness(255);

  // Simple neopixel test
  uint32_t colors[] = { 0x000000, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 };
  for( size_t color=0; color<sizeof(colors)/sizeof(*colors); color++ ) {
    for( unsigned pixel=0; pixel<NUM_PIXELS; pixel++ ) {
      pixels.setPixelColor(color&1 ? NUM_PIXELS-1-pixel : pixel, colors[color]);
      pixels.show();
      delay(500/NUM_PIXELS); // Each color iteration lasts 0.5 seconds
    }
  }

  // Setup the calculation values
  setupDefaults();
  EEPROM.begin(sizeof(eeprom_t));
  getEeprom();
  setupAnimation();

  Serial.println("\nBooted " VERSION);
}


// Worker loop, updates animation data and displays it on neopixels
void loop() {
  unsigned long t_ms = millis();

  // Online web update
  updaterHandle();

  // Calcuate new animation values
  set_animation_pixels(t_ms);

  // Update neopixel strip
  pixels.show();

  // Keep constant loop interval, if possible
  unsigned long wait_ms = INTERVAL_MS + t_ms - millis();
  delay(wait_ms <= INTERVAL_MS ? wait_ms : 0); // delay(0) reduces power?
}
