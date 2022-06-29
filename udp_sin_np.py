#!/usr/bin/python

# Example sending color pixels to NeoXmas via UDP
# Red, green and blue sine waves interfering, calculated with numpy

import socket
from time import sleep
from datetime import datetime
import numpy as np

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
freq = 1 / looptime       # base frequency for one wave over all leds

start = datetime.now()
now = start
array = np.ndarray((leds, 4))
array[0] = np.arange(leds)
t = np.linspace(0, looptime, leds, endpoint=False)
a_max = amplitude + offset
while True:
  t0 = (now - start).total_seconds()

  array[1] = amplitude * np.sin(2 * np.pi * freq * 2 * (t0+t)) + offset
  array[2] = amplitude * np.sin(2 * np.pi * freq * 3 * (t0-t)) + offset
  array[3] = amplitude * np.sin(2 * np.pi * freq * 5 * (t0+t)) + offset

  array[1:] = np.round(array[1:] * array[1:] / a_max)
  MESSAGE = array.tobytes(order='F')

  sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))

  prev = now
  now = datetime.now()
  delay = interval - (now - prev).total_seconds()
  if delay > 0:
    sleep(delay)
