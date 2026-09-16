#  Singularity Firmware

A custom firmware for M5Stack Cardputer-ADV (ESP32-S3) built around a lua interpreter.

## ⚠️ Disclaimer

A micro SD card is required for use of this firmware

## Hardware

- M5Stack Cardputer-Adv (ESP32-S3)
- microSD card (32GB recommended, FAT32)
- Optional: Grove IR receiver(/transmitter) module for IR capture

## Flashing

> The following guide is for ESPWebTool for other flashers it may differ

To flash the CardputerADV with the firmware first download a SGFirmware.bin from releases

REMOVE THE SD CARD BEFORE FLASHING IT MAY CORRUPT IF NOT REMOVED

1. Open the [ESPWebTool](https://esptool.spacehuhn.com/)
2. Check if the SD card is REMOVED
3. Hold G0 Button and connect the usb-c cable
4. Press Connect and select the port
5. If there are any then delete all the current files
6. Press ADD and upload the downloaded .bin file (set the adress to 00000000)
7. Press Program and wait untill the flashing finishes
8. ...and you are done! disconnect the cable and boot the device normally


> A web flasher for this firmware is planned to be made in the future

## Building

Built with [PlatformIO](https://platformio.org/). Framework: Arduino.

> (just use platformio im not giving you a tutorial here)

## Lua API

see documentation README inside Lua Apps folder

## SD Card Structure

example structure can be seen under /sd_files README

## License

GNU GENERAL PUBLIC LICENSE license