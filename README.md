# SoundOut
SoundOut is a social music system that makes music more democratic, more social, and more fun. Powered by ESP-mesh, SoundOut allows everyone to have their voice heard by selecting their favourite genre on a wearable smart glowstick, and broadcast their choice to the world by changing the colour of the glowstick to match your chosen genre.

Whether it's for a club, a party, or anything else, SoundOut hopes to change the way we interact with music and with eachother. 

### Use Case
Everyone at a party or club is given a smart glowstick, on which they can select what genre they feel like hearing simply by turning a knob. The glowsticks communicate using ESP-mesh, and the controller or `Root` node takes a snapshot and uses that data to communicate with the Spotify API to give reccomendations and play them.

The idea of the project is to make music choice more democratic, take control back from DJs and speaker-hogs, and to create interactions between people as they learn more about the people around them and their taste in music.

### Overview
![schematic](pics/schematic.png)

As you can see there are two main parts in the system, the glowstick and the controller.
  1. The Glowstick -> The part that has all the LEDs connected and that you wear around your neck
  2. The Controller ->  The part that collects all the data and then chooses which songs to play

## List of Materials

The descriptions in parenthesis are what we used to build/develop the system.

__Controller__

The controller makes use of an ESP32 as its root node which has a UART connection to talk to a Raspberry Pi which then does all the API interactions

![controller_parts](/pics/controller.png)

  - Raspberry Pi (3B+)
  - A power source (USB to Micro-USB)
  - HDMI Cable to connect Pi to screen
  - A screen
  - USB Mouse & Keyboard
  - A connection to a sound system (3.5mm Aux cord)
  - 3 wires with Male-Female headers

  - An ESP32 board (ESP DevKit-C)
  - A power source (USB to Micro-USB)

__A Glowstick Node__

ESP-Mesh supports up to [1000 nodes](https://www.espressif.com/en/products/software/esp-mesh/overview). But to get your party started here are the materials needed for one individual node:
  - An ESP32 DevKit. Both the ESP DevKit-C and the NodeMCU ESP-32S worked for us.
  - A power source for the LEDs, ideally at 12v but 9v works too, although it won't be a bright as 12v.
  - A power source for the ESP32. We powered it over USB, using either a USB battery bank, or the 9v battery (but it must be stepped down to the correct volatage!). 
  - A "dumb" (non-addressable) strip of RGB LEDs. We used 4 segments of about 10cm each on our prototype.
  - 3 N-Channel MOSFETs – we used STMicroelectronics P16NF06N-Ch 60 Volt 16 Amp MOSFETs, but any MOSFET in a TO-220 package should work. One is needed for each channel (R/G/B) of the LED strips. 
  - A rotary switch; to select the genre. The number of positions should be at least as many as the number of genres you want (our prototype has 6 genres).
  
  
## Installation Instructions
Individual instructions on how to install and wire pieces together can be found in the `mesh` and `player` folders. It also outlines the software environment needed to develop 

## Development History
Take a look at [Oliver's](https://cs.anu.edu.au/courses/china-study-tour/news/#oliver-johnson) and [David's](https://cs.anu.edu.au/courses/china-study-tour/news/#david-horsley) blogs. There you'll find answers to questions such as _"Why did we choose ESP-IDF over Arduino?"_ and _"Why we decided to use ESP-Mesh (Wifi) instead of BLE Mesh?"_

At the moment our prototype isn't incredible robust. If you want to get involved try extending the nodes to use extra genres, or implement a bit of data visualisation. We've got the foundations in the code but it would be really intersting to try and make it more "plug and play"

Fork a copy, raise an issue or a pull request and we can take it from there.

## If you're stuck...
We're very happy to help you out as much as possible, but the best thing that you can do is have a look at some of the resources that we used to develop the system:
  - [Neil Kolban's Book on ESP32](https://leanpub.com/kolban-ESP32)
  - [Espressif IoT Development Framework](https://github.com/espressif/esp-idf) and associated [docs](https://docs.espressif.com/projects/esp-idf/en/latest/)
  - [Espressif Mesh Development Framework](https://github.com/espressif/esp-mdf) and associated [docs](https://docs.espressif.com/projects/esp-mdf/en/latest/index.html)
  - [Spotify API Guides](https://developer.spotify.com/documentation/web-api/)
  - [Spotify Python Framework, spotipy](https://github.com/plamere/spotipy)
  - [Librespot](https://github.com/librespot-org/librespot)

You can also contact us through email and we'll try to get back to you:

Oliver: u6406755(at)anu.edu.au
David:  u6(at)anu.edu.au
