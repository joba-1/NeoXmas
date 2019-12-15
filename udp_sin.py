#!/usr/bin/python

# Example sending color pixels to NeoXmas via UDP
# Red, green and blue sine waves interfering

import socket
from time import sleep, time
from math import pi, sin

UDP_IP = socket.gethostbyname("neoXmas")
UDP_PORT = ord('N') << 8 | ord('X')
print("UDP target IP:", UDP_IP)
print("UDP target port:", UDP_PORT)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
millis = lambda: int(time() * 1000)

delay=0.03

amplitude_max = 127 # uint8_max / 2
looptime = 3000     # ms
leds = 25           # leds

amplitude  = amplitude_max
offset     = amplitude_max
wavelength = leds
phaseshift = 2.0 * pi / wavelength # phase shift between leds for one full wave 
frequency  = 2.0 * pi / looptime   # frequency for one wave per looptime

amplitude_red    = amplitude
offset_red       = offset
frequency_red    = frequency
phaseshift_red   = phaseshift * -1

amplitude_green  = amplitude
offset_green     = offset
frequency_green  = frequency
phaseshift_green = phaseshift * 3

amplitude_blue   = amplitude
offset_blue      = offset
frequency_blue   = frequency
phaseshift_blue  = phaseshift * 2

while True:
  now = millis()
  MESSAGE = bytes()

  for led in range(0, leds):

    red   = amplitude_red   * sin( frequency_red   * now + phaseshift_red   * led ) + offset_red
    green = amplitude_green * sin( frequency_green * now + phaseshift_green * led ) + offset_green
    blue  = amplitude_blue  * sin( frequency_blue  * now + phaseshift_blue  * led ) + offset_blue
    
    red   = (red   * red  ) / (offset + amplitude_max)
    green = (green * green) / (offset + amplitude_max)
    blue  = (blue  * blue ) / (offset + amplitude_max)
    
    MESSAGE += bytes([led, int(red+0.5), int(green+0.5), int(blue+0.5)])

  sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
  sleep(delay)
