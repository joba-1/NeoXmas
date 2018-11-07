#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include <math.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <stdlib.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WlanConfig.h>

// Network stuff
#define SSID             WlanConfig::Ssid
#define PASS             WlanConfig::Password
#define NAME             "NeoPendulum"
#define VERSION          NAME " " __DATE__ " " __TIME__
#define ONLINE_LED_PIN   D4

// Neopixel stuff
#define PIXEL_PIN        D5
#define NUM_PIXELS       50
#define NEO_CONFIG       (NEO_RGB+NEO_KHZ800)
#define MAX_ACCEL_PIXELS (NUM_PIXELS/4)

#define PENDULUM_COLOR   0x22ff22
#define ACCEL_COLOR      0x880022
#define BACKGROUND_COLOR 0x000022

// Update interval. Increase, if you want to save time for other stuff...
#define INTERVAL_MS      0

// Change, if you modify eeprom_t in a backward incompatible way
#define EEPROM_MAGIC     (0xabcd1234)

// EEPROM data
typedef struct {
  float phi0, g, l;
  uint32_t pd, ac, bg, magic;
} eeprom_t;

// Physics data
float Phi0, g, l, omega, s0, v0, a0;
unsigned long T_ms, t0_ms;

// Pendulum colors
uint32_t pd_color;
uint32_t ac_color;
uint32_t bg_color;


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_CONFIG);

ESP8266WebServer web_server(80);

ESP8266HTTPUpdateServer esp_updater;


// Builtin default settings, used if Eeprom is erased or invalid
void setupDefaults() {
  Phi0 = (M_PI/8);       // Phi(max) = 22.5 degrees
  l = 2.00;              // length in m
  g = 9.81;              // g force in m/s^2
  pd_color = PENDULUM_COLOR;
  ac_color = ACCEL_COLOR;
  bg_color = BACKGROUND_COLOR;
}


// Erase saved settings
void clearEeprom() {
  eeprom_t data {0};
  EEPROM.put(0, data);
  EEPROM.commit();
}


// Save current settings permanently
void setEeprom() {
  eeprom_t data { Phi0, g, l, pd_color, ac_color, bg_color, EEPROM_MAGIC };
  EEPROM.put(0, data);
  EEPROM.commit();
}


// Read previously saved settings
void getEeprom() {
  eeprom_t data;
  EEPROM.get(0, data);
  if( data.magic == EEPROM_MAGIC ) {
    Phi0 = data.phi0;
    l = data.l;
    g = data.g;
    pd_color = data.pd;
    ac_color = data.ac;
    bg_color = data.bg;
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


// Call this after g or l have been changed
void setupPhysics() {
  omega = sqrt(g/l);
  s0 = l * Phi0;
  v0 = s0 * omega;
  a0 = v0 * omega;
  float T = 2 * M_PI / omega;
  T_ms = (unsigned long)(T * 1000);
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

  // Call this page to clear the saved settings (physics and color)
  web_server.on("/clear", []() {
    clearEeprom();
    setupDefaults();
    setupPhysics();
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
      { "phi", 'f', &Phi0     },
      { "g",   'f', &g        },
      { "l",   'f', &l        },
      { "p",   'u', &pd_color },
      { "a",   'u', &ac_color },
      { "b",   'u', &bg_color } };

    bool ok = true;    // So far all processed URI parameters were ok
    int processed = 0; // Count processed URI parameters

    if( web_server.args() == 0 ) {
      DynamicJsonBuffer jsonBuffer(200);
      JsonObject& root = jsonBuffer.createObject();
      root["version"] = VERSION;
      JsonObject& cfg = root.createNestedObject("cfg");
      cfg["phi"] = Phi0;
      cfg["l"] = l;
      cfg["g"] = g;
      JsonObject& color = cfg.createNestedObject("colors");
      color["p"] = pd_color;
      color["a"] = ac_color;
      color["b"] = bg_color;
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
        setupPhysics();
        setEeprom();
        web_server.send(200, "text/plain", "ok: saved\n");
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

  // Catch all page, gives a hint on valid URLs
  web_server.onNotFound([]() {
    web_server.send(404, "text/plain", "error: use "
      "/cfg?parm=value[&parm=value...] /reset, /clear, /version or "
      "post image to /update\n");
  });

  web_server.begin();

  MDNS.addService("http", "tcp", 80);
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


// Set pendulum pixels according to physics data
void set_pendulum_pixels( float s, float a ) {
  unsigned long pixel_color;
  unsigned upper_limit, lower_limit;

  unsigned pendulum_pos = (unsigned)round(s);   // Pendulum pixel, begin of acc
  unsigned accel_pos    = (unsigned)round(s+a); // Last acceleration line pixel

  // Recalculate and set colors of all pixels
  for( unsigned pixel=0; pixel<NUM_PIXELS; pixel++ ) {
    if( pixel == pendulum_pos ) {
      pixel_color = pd_color;
    }
    else {
      lower_limit = min(accel_pos, pendulum_pos);
      upper_limit = max(accel_pos, pendulum_pos);
      if( (pixel > lower_limit) && (pixel < upper_limit) ) {
        pixel_color = ac_color;
      }
      else {
        pixel_color = bg_color;
      }
    }

    pixels.setPixelColor(pixel, pixel_color);
  }
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
  uint32_t colors[] = { 0x000000, 0xff0000, 0x00ff00, 0x0000ff };
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
  setupPhysics();
  t0_ms = millis();

  Serial.println("\nBooted " VERSION);
}


// Worker loop, updates pendulum physics data and displays it on neopixels
void loop() {
  unsigned long t_ms = millis();

  // Online web update
  updaterHandle();

  // Make sure t is between 0 and T to avoid overflow problems
  while( T_ms < (t_ms - t0_ms) ) {
    t0_ms += T_ms;
  }

  // Calcuate new pendulum values
  float t = (t_ms - t0_ms) / 1000.0;
  float s = s0 * sin(omega*t - Phi0);
  // float v = v0 * cos(omega*t - Phi0);
  float a = a0 * -sin(omega*t - Phi0);

  // Transfer new pendulum values to neopixels (position and acceleration bar)
  set_pendulum_pixels(
     s / s0 * (NUM_PIXELS-1)/2.0 + (NUM_PIXELS-1)/2.0,
     a / a0 * MAX_ACCEL_PIXELS);

  // Update neopixel strip
  pixels.show();

  // Keep constant loop interval, if possible
  unsigned long wait_ms = INTERVAL_MS + t_ms - millis();
  if( wait_ms <= INTERVAL_MS ) {
    delay(wait_ms);
  }
}
