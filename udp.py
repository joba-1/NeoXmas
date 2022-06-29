#!/usr/bin/python

# Example sending color pixels to NeoXmas via UDP

import socket
from time import sleep

UDP_IP = socket.gethostbyname("neoXmas")  # or as string like "172.20.10.14"
UDP_PORT = ord('N') << 8 | ord('X')
print("UDP target IP:", UDP_IP)
print("UDP target port:", UDP_PORT)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

leds=50
near=4
maxi=255
delay=0.03

# Red and green band move in opposite directions on blue background
while False:
  for j in range(0, leds):
    MESSAGE = ""
    for i in range(0, leds):
      d = abs(i-j);
      if d < near:
        n = maxi / near * d
      else:
        n = maxi
      if d <= near:
        MESSAGE = MESSAGE + chr(i)        + chr(0)      + chr(maxi-n)    + chr(n)
        MESSAGE = MESSAGE + chr(leds-i-1) + chr(maxi-n) + chr(0)         + chr(n)

    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    sleep(delay)

# Red, green and blue blocks of pixels moving
block = 9
while False:
  for j in range(0, 3*block):
    MESSAGE = ""
    for i in range(0, leds):
      mode = (i + j) % (3*block)
      i = leds - i - 1 # change direction
      if mode < block:
        MESSAGE = MESSAGE + chr(i) + chr(maxi) + chr(0)    + chr(0)
      elif mode < 2*block:
        MESSAGE = MESSAGE + chr(i) + chr(0)    + chr(maxi) + chr(0)
      elif mode < 3*block:
        MESSAGE = MESSAGE + chr(i) + chr(0)    + chr(0)    + chr(maxi)
      else:
        print(mode)
    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    sleep(delay*2)

# Italian flag
block = 6
while True:
  for j in range(0, 4*block):
    MESSAGE = ""
    for i in range(0, leds):
      mode = (i + j) % (4*block)
      i = leds - i - 1 # change direction
      if mode < block:
        MESSAGE = MESSAGE + chr(i) + chr(maxi) + chr(0)    + chr(0)
      elif mode < 2*block:
        MESSAGE = MESSAGE + chr(i) + chr(maxi) + chr(maxi) + chr(maxi)
      elif mode < 3*block:
        MESSAGE = MESSAGE + chr(i) + chr(0)    + chr(maxi) + chr(0)
      elif mode < 4*block:
        MESSAGE = MESSAGE + chr(i) + chr(0)    + chr(0)    + chr(0)
      else:
        print(mode)
    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    sleep(delay*2)
