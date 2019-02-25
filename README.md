# iot-stick
Decide on what set to play by wearing a Glowstick, powered by ESP Mesh. 

### Use Case
Give everyone at a party or club one of these glowsticks. On each glowstick is a knob to select what genre they're feeling. Each of these glowsticks are networked via ESP Mesh. The controller or `Root` node takes a snapshot and uses that data to communicate with the Spotify API to give reccomendations and then automatically play them.

## Overview
![schematic](pics/schematic.png)

### Github Structure
There are two moving parts in the system.
  1. The Glowstick -> The part that has all the LEDs connected and that you wear around your neck
  2. The Controller -> 


### Development History
Take a look at [Oliver's](https://cs.anu.edu.au/courses/china-study-tour/news/#oliver-johnson) and [David's](https://cs.anu.edu.au/courses/china-study-tour/news/#david-horsley) blogs. There we outline the process from ideation to implementation of our devices. It may also explain some of the design choices (such as moving away from BLE Mesh that we thought was great, and deciding to use esp-idf over arduino)

## Hardware Required

## Installation Instructions

