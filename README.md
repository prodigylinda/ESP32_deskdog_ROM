# ESP32 DeskDog ROM

## Project Overview

**ESP32 DeskDog ROM** is a customized ESP32 firmware project based on the original **esp_hi** open-source project.

This project is developed for an ESP32-based desktop robot, with the original project's core functionality preserved while adding **voice message text display functionality** instead of emoji, and it can be use to see the reply of the robot dog clearly. This allows the robot to display the corresponding text content while interacting through voice messages.

The repository also includes a ready-to-flash firmware binary for users who want to deploy the customized firmware without rebuilding the project from source.

## Features

* Based on the original **esp_hi** open-source project
* Designed for ESP32-based desktop robots
* Added **voice message text display**
* Preserves the original project's core functionality
* Includes a ready-to-flash `.bin` firmware file
* Includes project dependencies and configuration files

## Credits

This project is based on the following open-source project:
* **Original Project:** [esp_hi]([Project Link](https://github.com/78/xiaozhi-esp32))
* **Original Author:** [Xiaoxia]

## Repository Contents
The repository includes the files required to use and further develop the customized firmware:
* Source code
* `managed_components/` — project dependencies
* `sdkconfig` — ESP-IDF project configuration
* `dependencies.lock` — dependency version information
* `xiaozhi-esp32-dog.bin` — pre-built firmware for flashing

## How to Flash
The pre-built firmware can be used directly to flash the ESP32 device.
Detailed flashing instructions will be added later.
1. Connect the ESP32 device to your computer.
2. Prepare an appropriate ESP32 flashing tool.
3. Select `xiaozhi-esp32-dog.bin`.
4. Flash the firmware to the device.
5. Restart the device after flashing is complete.
> Make sure the firmware is compatible with your specific ESP32 board before flashing.

## Build from Source
To modify or rebuild the firmware, clone this repository and open it in an ESP-IDF development environment.
```bash
git clone https://github.com/prodigylinda/ESP32_deskdog_ROM.git
cd ESP32_deskdog_ROM
```
You can then build and flash the project using the appropriate ESP-IDF commands for your target board.

## Disclaimer
This is a personal modification of the original open-source project.

