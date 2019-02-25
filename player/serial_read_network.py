# Adapted from instructables.com - Read and write from serial port with Raspberry Pi

# Will sample the UART readout 10 times before exiting
import time
import serial
import math

genres = ['pop','electro','r-n-b','rock','indie','disco','jazz']
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
    #
    while (1):
        x=ser.readline().decode('utf-8')
        print(str.isdigit(x))
        print(x)
        if(x.isdigit()):
            g = genres[int(x) % len(genres)]
            print(g)
            f.write(g+"\n")
            f.flush()
        counter += 1
readUART()