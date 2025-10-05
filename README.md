# LD2410Async - Asynchronous ESP32 Arduino Library for the LD2410 mmWave Radar Sensor

[![Arduino Library](https://www.ardu-badge.com/badge/LD2410Async.svg)](https://www.ardu-badge.com/LD2410Async)  [![GitHub release](https://img.shields.io/github/release/lizardking/LD2410Async.svg?logo=github)](https://github.com/lizardking/LD2410Async/releases/latest)
  [![LD2410Async Build](https://github.com/lizardking/LD2410Async/actions/workflows/build.yml/badge.svg)](https://github.com/lizardking/LD2410Async/actions/workflows/build.yml)  [![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue?logo=github)](https://lizardking.github.io/LD2410Async/)
  ![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

![LD2410](/gfx/LD2410.jpg)

## Introduction

The **LD2410Async** library provides an asynchronous interface for the **Hi-Link LD2410** human presence radar sensor on Arduino/ESP32 platforms.

LD2410Async runs in the background using FreeRTOS tasks. This allows your main loop to remain responsive while the library continuously processes radar data, handles configuration, and manages timeouts.  

The library has the following functionality:  

- **All LD2410 commands supported** are available in the library.
- **Full access to sensor data** - all data that is sent by the sensor is available.  
- **Async command API** - send commands (e.g., request firmware, change settings) without blocking the main loop().
- **Callbacks** - are executed when detection data has arrived or when commands complete.
- **Non-blocking operation** - sensor data is parsed in the background, no polling loops required.  
 
---

## Documentation

Full API and usage documentation is generated automatically with Doxygen and published on GitHub Pages:  

**[View the LD2410Async Documentation](https://lizardking.github.io/LD2410Async/)**

The docs include:
- A complete [LD2410Async class reference](https://lizardking.github.io/LD2410Async/classLD2410Async.html)
- Full details for all functions, callbacks, and data structures.

### Direct links to main sections

- [Installation](https://lizardking.github.io/LD2410Async/Installation.html)  
- [Async Commands & Processing](https://lizardking.github.io/LD2410Async/Async_Commands_And_Processing.html)  
- [Data Structures](https://lizardking.github.io/LD2410Async/Data_Structures.html)  
- [Operation Modes](https://lizardking.github.io/LD2410Async/Operation_Modes.html)  
- [Inactivity Handling](https://lizardking.github.io/LD2410Async/Inactivity_Handling.html)  
- [Examples](https://lizardking.github.io/LD2410Async/Examples.html)  
- [Best Practices](https://lizardking.github.io/LD2410Async/BestPractices.html)  
- [Troubleshooting Guide](https://lizardking.github.io/LD2410Async/Troubleshooting.html)
---
 
## Installation

You can install this library in two ways:

### 1. Using Arduino Library Manager (recommended)
1. Open the Arduino IDE.  
2. Go to **Tools - Manage Libraries...**.
3. Search for **LD2410Async**.  
4. Click **Install**.  

### 2. Manual installation
1. Download this repository as a ZIP file.  
2. In the Arduino IDE, go to **Sketch - Include Library - Add .ZIP Library...**, or
   unzip the file manually into your Arduino `libraries` folder:  
   - Windows: `Documents/Arduino/libraries/`  
   - Linux/macOS: `~/Arduino/libraries/`  
3. Restart the Arduino IDE.  

---

## Basic Usage

Include the library, create an instance for your radar, and start it with `begin()`.  
Register a callback to get notified when new detection data arrives.

Check the [examples](docu/Examples.html) section for more examples.

### Simply sensing presence

```cpp
#include <LD2410Async.h>

// Define the serial port connected to the LD2410 (example: Serial1 on ESP32)
#define RADAR_RX_PIN 16
#define RADAR_TX_PIN 17
HardwareSerial RadarSerial(1);

// Create radar instance
LD2410Async radar(RadarSerial);

// Detection callback function
void onDetectionData(LD2410Async* sender, bool presenceDetected, byte userData) {
  if (presenceDetected) {
    Serial.println("Presence detected!");
  } else {
    Serial.println("No presence.");
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize radar serial
  RadarSerial.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

  // Start radar task
  if (radar.begin()) {
    Serial.println("Radar started successfully.");
  } else {
    Serial.println("Failed to start radar.");
  }

  // Register detection callback
  radar.onDetectionDataReceived(onDetectionData, 0);
}

void loop() {
  // Your main loop stays free for other tasks
  delay(1000);
  Serial.println("Main loop running...");
}
```

---
 

## License

This library is released under the **MIT License**.  
You are free to use, modify, and distribute it, including for commercial purposes, with no restrictions.  

See the [LICENSE](LICENSE) file for details.
