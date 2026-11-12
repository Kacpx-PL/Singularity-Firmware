#  Singularity Firmware

A custom firmware for M5Stack Cardputer-ADV (ESP32-S3) built around a lua interpreter.

## ⚠️ Disclaimer

A micro SD card is required for use of this firmware

## Hardware

- M5Stack Cardputer-Adv (ESP32-S3)
- microSD card (32GB recommended, FAT32)
- Optional: Grove IR receiver module for IR capture

## Flashing

### [Singularity Flasher](https://kacpx-pl.github.io/Singularity-Firmware/)

**Remove** the sd card before flashing

1. Hold G0 Button and connect the usb-c cable
2. Press Connect and select the port
4. Select the firmware version 
5. Press Flash and wait untill the flashing finishes
6. ...and you are done! disconnect the cable and boot the device normally

> Note this flasher is in beta there may be bugs
> If you dont want to use it, download the files from releases

## Building

Built with [PlatformIO](https://platformio.org/). Framework: Arduino.

> (just use platformio im not giving you a tutorial here)

## Lua API

see documentation README inside Lua Apps folder

## SD Card Structure

example structure can be seen under /sd_files README

## License

GNU GENERAL PUBLIC LICENSE license