# ESP32 DeskDog ROM（For small size TFT LCD)

**ESP32 DeskDog ROM** is a customized firmware based on the open-source [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) project, designed for ESP32-powered desktop robot dogs.

This project keeps the core functionality of the original firmware while adding **real-time AI voice reply text display** on the device screen. This allows users to clearly read the robot dog's responses while interacting through voice.

## Features

* Real-time AI reply text display
* Voice interaction with cloud-based AI services
* Emotional expression animations
* Wi-Fi connectivity
* Support for cloud LLM services such as Qwen and DeepSeek
* OTA-ready
* Pre-built `.bin` firmware files included
* Modified source code included for further development

## Firmware

The repository provides pre-built firmware files that can be flashed directly to a compatible ESP32 device.
> **Note:** If multiple `.bin` files are provided, all required firmware files must be flashed to their corresponding addresses. Please check the included firmware configuration or flashing instructions before flashing.
> 
## Flashing
You can flash the firmware using an ESP32 flashing tool such as **ESP-IDF**, **esptool**, or another compatible ESP32 flash utility.
The exact flash addresses depend on the firmware configuration and board being used.
**Make sure to back up your original firmware before flashing.**
## Project Structure

```text
ESP32_deskdog_ROM/
├── README.md
├── firmware/
│   ├── bootloader.bin
│   ├── partition-table.bin
│   ├── ota_data_initial.bin
│   └── firmware.bin
└── source/
    └── ...
```

The actual filenames may vary depending on the build configuration.

## Based On

This project is based on **xiaozhi-esp32**, an open-source ESP32 AI voice interaction project developed by Xiaoxia.
Original project:

https://github.com/78/xiaozhi-esp32

Please refer to the original repository for the original project's license and terms.


## Disclaimer
This is a personal modification of the original project and is provided **as-is** for learning, development, and personal use.
I am not affiliated with the original xiaozhi-esp32 project or its developers.
Use the firmware at your own risk. Make sure the firmware is compatible with your hardware before flashing.

## License
This repository follows the applicable license requirements of the original **xiaozhi-esp32** project.
Please check the original repository for the complete license information.
