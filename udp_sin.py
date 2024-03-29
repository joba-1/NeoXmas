#!/usr/bin/python

# Example sending color pixels to NeoXmas via UDP
# Red, green and blue sine waves interfering

import socket
from time import sleep
from datetime import datetime
from math import pi, sin

UDP_IP = socket.gethostbyname("neoXmas")
UDP_PORT = ord('N') << 8 | ord('X')
print("UDP target IP:", UDP_IP)
print("UDP target port:", UDP_PORT)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

fps = 30                  # update frequency in Hz
interval = 1.0 / fps      # loop time
offset = 255 / 2.0        # moves center of wave to middle of available range of 0 - 255
amplitude = 255 - offset  # wave around offset
looptime = 15.0           # s of one wave going through all leds 
leds = 50                 # used leds
dt = looptime / leds      # time shift between leds
freq = 1 / looptime       # base frequency for one wave over all leds

first = True
start = datetime.now()
now = start
while True:
  t0 = (now - start).total_seconds()
  MESSAGE = ""

  for led in range(leds):
    t_fwd = t0 + led * dt
    t_bck = t0 - led * dt

    red   = amplitude * sin(2 * pi * (freq*2) * t_fwd) + offset
    green = amplitude * sin(2 * pi * (freq*3) * t_bck) + offset
    blue  = amplitude * sin(2 * pi * (freq*5) * t_fwd) + offset
    
    # linearize perceived brightness
    red   = (red   * red  ) / (offset + amplitude)
    green = (green * green) / (offset + amplitude)
    blue  = (blue  * blue ) / (offset + amplitude)

    MESSAGE += chr(led) + chr(int(round(red))) + chr(int(round(green))) + chr(int(round(blue)))

  sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))

  prev = now
  now = datetime.now()
  delay = interval - (now - prev).total_seconds()
  if delay > 0:
    sleep(delay)
  first = False
