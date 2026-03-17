# Laptrinhnhung - Smart Home Automation System

## Overview
This project is a comprehensive smart home/building automation system called "Miki - Light Hub". It consists of embedded firmware for microcontroller units and a web-based dashboard for monitoring and control.

## Components

### Firmware
- **ESP32 Firmware**: Acts as the gateway with WiFi connectivity, web server, RFID management, LCD display, audio playback, Google Sheets logging, and MQTT communication.
- **Arduino Mega Firmware**: Controls various actuators (doors, curtains, fans, LEDs, buzzers) and monitors sensors (gas, fire, temperature, humidity, motion, etc.).

### Web Dashboard
- Built with PHP, featuring a modern UI using Tailwind CSS.
- Provides real-time monitoring and control of the smart home system.
- Includes AI controller integration for advanced automation features.

## Features
- RFID-based access control
- Environmental monitoring (temperature, humidity, gas, fire detection)
- Automated actuators (doors, curtains, fans, lights)
- Emergency button and alert system
- Web-based remote control and monitoring
- Data logging to Google Sheets
- Real-time communication between devices

## Hardware Requirements
- ESP32 microcontroller
- Arduino Mega 2560
- Various sensors (DHT, gas sensor, fire sensor, ultrasonic, LDR, etc.)
- Actuators (servos, stepper motors, relays, buzzers, LEDs)
- RFID reader
- LCD displays
- DFPlayer Mini for audio

## Software Requirements
- Arduino IDE for firmware compilation
- PHP 7+ for web dashboard
- Web server (Apache/Nginx) for hosting
- MySQL database (if needed for data storage)

## Installation
1. Clone this repository
2. Upload ESP32 firmware to ESP32 board using Arduino IDE
3. Upload Mega firmware to Arduino Mega board using Arduino IDE
4. Configure web server for the `laptrinhnhung` directory
5. Update configuration files with your network and API credentials

## Usage
- Access the web dashboard via your browser
- Monitor sensor data and system status
- Control actuators remotely
- Configure automation rules

## Contributing
Contributions are welcome. Please fork the repository and submit pull requests.

## License
This project is open-source. Please check individual components for specific licenses.