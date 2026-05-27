# NOP100/firmware

```NOP100.cpp``` implements a runnable, extensible, firmware for
[NOP100-based hardware](../hardware/README.md).

The files defines.h, definitions.h, includes.h, loop.h and setup.h
allow specialisation of NOP100 to a particular application by
extending and overriding some core functionality.

If specialisation is absent, the firmware creates an NMEA 2000
device with Class Code 10 (System Tools) and Function Code 130
(Diagnostic).
The created device is essentially passive, but will respond to
PGN 126208 Group Function Handler commands to update its default
instance address.

Supplying functional content in the specialisation files allows the
construction of a useful NMEA 2000 application.
The recommended way of doing this is to create a folder under the
```modules/``` directory for a planned application and to populate this
with specialisation files that become the target of symbolic links in
the NOP100 folder.

By way of illustration, the ```modules/NOP100-SIM``` folders contains
code which specialises the NOP100 firmware so that it acts as an NMEA
2000 switch input module.
The extended firware supports one or two
[MikroE 5981]()
modules as the external switch input interface.






The modules/CLICK5981 folder, for example, 
Most useful applications will exploit interfaces implemented by
MikroBus modules which plug into the NOP100 motherboard and it is
[SIM108 - NMEA 2000 switch input module](https://www.github.com/preeve9534/SIM108/)
project is based on NOP100 and gives a working example of a real-world
application.

## Module configuration

Modules based on NOP100 are intended to support structured
configuration updates via PGN126208 Command Group Function or by use
of the hardware DIL switch which allows byte-level updates to binary
configuration data. This restricts the configuration data cache to a
size of 256 bytes with 254 bytes available to the application.

The Teensy microcontroller has a about 1KB of EEPROM available and if
you want to access more than 256 bytes of this storage then you will
need to override NOP100's simple configuration model with a more
elaborate one of your own.

If you do this, make sure that your application configuration data
leaves address locations 0 and 1 for use by NOP100.

| Address | Saved value |
| ---:    | :---        |
| 0       | CAN source address. Not available for application use. |
| 1       | Module instance number. Not available for application use. |
| 2...    | Available for application use. |

Entry of configuration data into NOP100 requires a value to be set on
the PCB's DIL switch and then confirmed by press and release of the PRG
button.
A short press of PRG signifies the entry of a data value; a long press
of PRG signifies entry of an address and, in this latter case, the
expectation is that address entry will promptly be followed by the
entry of a data value which should be stored at the pre-set address.

If a value is entered without a preceeding address then the entered
value is stored at address 1 and will become the module's new instance
number.

Storage locations with an address greater than or equal to two are
intended for application configuration data and entry of this data
requires the entry of an address followed by entry of the value to be
stored at this address.

When an address is entered, the transmit LED will flash rapidly
indicating that the system is waiting for entry of a data value.
If a data value is not entered within one minute the data entry protocol
will self-cancel (the transmit LED will stop flashing).

In ```NOP100.cpp``` the configuration process is handled by the
```prgButtonHandler(bool _state_, int _value_)``` function which is
called each time the PRG button is operated.
*state* will be true if the PRG button has been released or false if
the PRG button has been pressed.
In either case, *value* will be set to the current value read from the
DIL switch.

You can override NOP100's built-in behaviour by overriding the button
handler in ```module-declarations.inc```.
For example, the following code disables configuration entirely:
```
#define PRG_BUTTON_HANDLER
void prgButtonHandler(bool state, int value) {
   return;
}
```

## HOW TO

1. Create parent folder for your new application.
   ```
   $> mkdir n2k-projects
   $> cd n2k-projects
   ```
2. Clone a copy of the Arduino firmware build project and this
   project into separate sub-folders.
   ```
   $> git clone https://github.com/preeve9534/firmware-factory
   $> git clone https://github.com/preeve9534/NOP100
   ```
3. Make a directory for your new project and populate it.
   ```
   $> mkdir myapp
   $> cd myapp
   $> cp ../NOP100/NOP100.cpp myapp.cpp
   $> cp ../NOP100/*.inc .
   $> cd ..
   ```
   You can now delete the NOP100 project.
   ```
   rm -rf NOP100
   ```
4. Set up the build environment so that it points to your new
   project.
   ```
   $> cd firmware-factory/sketch
   $> rm src
   $> ln -s ../../myapp src
   $> cd ../..
   ```
   At this juncture you should be able to open the firmware-factory 
   project in your preferred build environment (I use Code) and
   compile a firmware for your application which does nothing. Well,
   not very much.
5. Edit the ```.inc``` files downloaded at (3) to tailor and
   extend the module firmware to the requirements of your
   application.  DO NOT MODIFY myapp.cpp.

