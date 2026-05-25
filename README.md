# NOP100

The **NOP100** project implements an
[NMEA 2000](https://en.wikipedia.org/wiki/NMEA_2000)
interface module with
[MikroBUS](https://www.mikroe.com/mikrobus)
and
[OneWire]()
interfaces.
Twin MikroBUS sockets allow specialisation of function by the addition of
[Click](https://www.mikroe.com/click)
expansion cards and appropriate software extensions.

[**NOP100** hardware](./hardware/README.md)
is based on the Teensy 4.0 micro-controller supported by a CAN
transceiver, parasitic power supply and a user configuration
interface.

[**NOP100** firmware](./firmware/README.md)
manages NMEA 2000 connectivity, user interaction and module
configuration and is easily extended by the addition of firmware
dedicated to interfacing any installed MikroBus and OneWire devices.

## Project organisation

```
NOP100/
  + firmware/            # NOP100 firmware...
    + modules/           # Folders for module specialisations
      + NOP100-SIM/        # Specialisation defining an NMEA switch input module
        + defines.h
        + definitions.h
        + includes.h
        + loop.h
        + setup.h
      + another module specialisation/
      + .../
    + NOP100.cpp         # NOP100 main application 
    + README.md
    + defines.h          # Symbolic link to a module specialisation
    + definitions.h      #   "       "
    + includes.h         #   "       "
    + loop.h             #   "       "
    + setup.h            #   "       "
  + hardware/            # NOP100 hardware...
    + gerber/            # Gerber files for PCB fabrication
    + kicad/             # Kicad design for the NOP100 motherboard
    + README.md
```
