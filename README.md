# ESP32 BLE Proximity Sensor for Philips Hue

ESP-IDF project for automating Philips Hue smart bulbs based on proximity detection. Utilizes HTTPS to send requests  securely to a local Philips Hue Bridge. Proximity detection is achieved by obtaining an average RSSI value for a BLE beacon from a BLE-enabled device (e.g. a smartphone) and using a set RSSI limit to determine if the device is within the set range of the ESP32.


## How to use
A number of settings must be configured through ESP-IDF's `menuconfig` in order to run this project. All required settings are listed under `Philips Hue Proximity Control Settings` as well as some advanced settings. Advanced settings are primarily for improving connection ability to WiFi APs with unstable connections.

## Why was this developed?
Currently, Philips Hue supports Smart Scenes which enable the bulbs to change settings based on the time of day but allows very little flexibility for enabling these scenes with physical buttons or app wigets. Philips Hue does, however, provide an API that allows for the enabling of Smart Scenes and more complex control of bulbs with HTTP requests. This project aims to fix the issue that many smart bulbs have of being complicated to turn on and off by allowing this to be automated simply by detecting if a BLE beacon from a device is within a set range of the ESP32, while also making Philips Hue Smart Scenes automatically activate.
