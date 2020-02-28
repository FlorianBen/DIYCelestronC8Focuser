# DIYCelestronC8Focuser (WIP)

## Disclaimer and read first

DIY focuser for the Celestron C8 telescope aka poor's man focuser. 

The main goal of this project is to provide a cheap solution for motorizing the focus rod of a Celestron C8 SCT.
Most of the work has been done using open source softwares.

The bill of material:

- Arduino Uno or Nano: 10€
- Case for the Arduino board and cable: 5€
- 28BYJ-48 motor and ULN2003 driver: 5€
- 3D printing service for the manufacturing: 40€ (or even less if you already have a 3D printer)

This project is a homemade attempt to create a focussing motor for my telescope and it is primitive compare to a commercial off the shelf focuser. Do not if you are not confident with what you are doing. High torque or forcing the limits on the focussing shaft may break the mirror/telescope. **THEREFORE, THE PRESENT RESOURCES ARE PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY**

## 3D model

The 3D parts are heavily inspired by this [design](https://www.thingiverse.com/thing:3552339/).
The parts were designed and assembled with FreeCAD. Each part has small footprint reducing the cost of 3D printing. Bolts/screws and nuts are possible for the part assembly as well as threaded inserts and threading tools.
Remember that the coils must be switched off if you intent to use PLA filament and own code to drive the stepper, otherwise high temperatures may damage the parts.

## Arduino code

The Arduino code uses two libraries:

- [AccelStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/) to control the stepper motor.
- [Vrekrer_scpi_parser](https://github.com/Vrekrer/Vrekrer_scpi_parser) to parse the incoming SCPI commands.

Be sure that both of these dependencies are installed in the arduino library folder.

A state machine drives the motor while the serial interface are read between loop iteration. 
The command are sent in ASCI format through the Arduino serial interface, usually `/dev/ttyAMC0`. The table below lists each available commands:

Command                   | Description
--------------------------|-------------------------------------
*IDN?                     | Ask for the identification string
FOCus:STEPper:ACcel float | Set the acceleration of the stepper
FOCus:STEPper:ACcel?      | Ask for the acceleration
FOCus:STEPper:RPMspeed    | Set the maximum speed of the stepper
FOCus:STEPper:RPMspeed?   | Ask for the maximum speed
FOCus:STEPper:Go int      | Move according to the given step.
FOCus:STEPper:GOto int    | Move to the given position.
FOCus:STEPper:Go?         | Ask if the stepper is running.
FOCus:STEPper:ABort       | Abort motion.
FOCus:STEPper:POSition?   | Ask for the current position.
FOCus:STEPper:RAZPOSition | Reset the position to zero.
FOCus:STEPper:MAXpos int  | Set the maximum position.
FOCus:STEPper:MAXpos?     | Ask for the maximum position.


## Indilib

An INDI driver is avaible for the focuser. The patch is avaible in the indi folder. First download INDI source files from here [INDI](https://github.com/indilib/indi) and apply the patch:

```sh
cd indi
git am --signoff -k < 0001-Support-for-an-poor-man-s-DIY-focuser-for-Celestron.patch
```

Then indi can be build as described in the [INDI Readme](https://github.com/FlorianBen/indi#building)
