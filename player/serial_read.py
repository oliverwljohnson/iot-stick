# Adapted from instructables.com - Read and write from serial port with Raspberry Pi
import time
import serial
import fs

f = open("reccomendations", "a")

ser = serial.Serial(
    port='/dev/serial0',
    baudrate= 115200,
    parity= serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
    )
counter=0

while 1:
    x=ser.readline()
    print (x)
    f.write("{"+time.time()+","+x+"}")
