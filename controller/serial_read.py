# Adapted from instructables.com - Read and write from serial port with Raspberry Pi

# Will sample the UART readout 10 times before exiting
import time
import serial
import math

def readUART():
    f = open("reccomendations", "w")

    ser = serial.Serial(
        port='/dev/serial0',
        baudrate= 9600,
        parity= serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
    )   
    counter=0
    time.sleep(1)
    while (counter < 1):
        x=ser.readline().decode('utf-8')
        print (x)
        t=str(math.floor(time.time()))+","+str(x)+"\n"
        f.write(str(t))
        f.flush()
        counter += 1