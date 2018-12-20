# NeoXmas
Animating a NeoPixel strip resembling sparks

## Required Hardware
* Runs on an ESP8266 (e.g. a NodeMCU) with a NeoPixel compatible RGB strip (the more pixels the better...)
* Needs two digital pins (or rather just one, the other is for the internal led to see the wlan connection status)
* Build your firmware with PlatformIO (adapt platformio.ini to match your upload config) or Arduino IDE
* You will need libraries ArduinoJson and Adafruit_Neopixel installed

## Optional WLAN
It does not need WLAN access to display the xmas spark animations, but it is still nice to have:
* Replace SSID and PASS (or WlanConfig.h) with your own WLAN settings
* It will use DHCP to get an IP address and show up on your network as `http://NeoXmas.local`
* Call it with curl/wget or a web browser to configure the device with its internal webserver - but no fancy gui, sorry :)
* For now you can switch the current animation mode. It should be easy to add new ones.
* Parameters are changed permanently in EEPROM/Flash until you erase them with `http://NeoXmas.local/clear`
* You can even update its firmware with a simple curl -vF 'image=@firmware.bin' NeoXmas.local/update

## Control the Strip via UDP
Send UDP packets to port 'NX' (20056) to control pixel colors.
An udp packet consists of blocks. At most one for each pixel you have.
Each block consists of one byte pixel number and 3 bytes with r, g and b values.
YOu dont need to update all pixels with each packet.
E.g. if you send bytes 0, 255, 0, 0,    1, 0, 128, 0,    2, 0, 64, 64, the current
animation will stop and you should see bright red, medium green and dark cyan as the first 3 pixels.
One cycle after sending the last udp packet, NeoPixel resumes with its current animation.
udp.py is an example program written in python that sends an animation via udp.

Have fun!
