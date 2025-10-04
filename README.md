# LD2410Async – Asynchronous ESP32 Arduino Library for the LD2410 mmWave Radar Sensor

[![Arduino Library](https://www.ardu-badge.com/badge/LD2410Async.svg)](https://www.ardu-badge.com/LD2410Async) [![LD2410Async Build](https://github.com/lizardking/LD2410Async/actions/workflows/build.yml/badge.svg)](https://github.com/lizardking/LD2410Async/actions/workflows/build.yml)! [License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

# This lib is still in development. Therefore breaking changes are likely. Use at at your own risk.


## Introduction

The **LD2410Async** library provides a asynchronous interface for the **Hi-Link LD2410** human presence radar sensor on Arduino/ESP32 platforms.  

LD2410Async runs in the background using FreeRTOS tasks. This allows your main loop to remain responsive while the library continuously processes radar data, handles configuration, and manages timeouts.  

The library has the following functionality:  

- **Non-blocking operation** – sensor data is parsed in the background, no polling loops required.  
- **Async command API** – send commands (e.g., request firmware, change settings) without blocking the main loop().
- **Callbacks** – are executed when detection data has arrived or when commands complete. If you only care about presence detection and dont need additional data, you only have to check the `presenceDetected` flag that is sent with the DetectionDataReceivedCallback. 
- **Full access to sensor data** – if you need more than just the basic `presenceDetected` information. All data that is sent by the sensor is available.  
- **All LD2410 commands** are available in the library.
 
---

## Installation

You can install this library in two ways:

### 1. Using Arduino Library Manager (recommended)
1. Open the Arduino IDE.  
2. Go to **Tools - Manage Libraries…**.  
3. Search for **LD2410Async**.  
4. Click **Install**.  

### 2. Manual installation
1. Download this repository as a ZIP file.  
2. In the Arduino IDE, go to **Sketch - Include Library - Add .ZIP Library…**, or  
   unzip the file manually into your Arduino `libraries` folder:  
   - Windows: `Documents/Arduino/libraries/`  
   - Linux/macOS: `~/Arduino/libraries/`  
3. Restart the Arduino IDE.  

---

## Documentation

Full API and usage documentation has been generated with Doxygen and available in the [`docu/`](docu/index.html) folder.  
Open [`index.html`](docu/index.html) in your browser to get started.

The docs also include a class reference for the [LD2410Async class](docu/classLD2410Async.html) and full details for all functions, callbacks, and data structures.

### Direct links to main sections

- [Installation](docu/Installation.html)  
- [Async Commands & Processing](docu/Async_Commands_And_Processing.html)  
- [Data Structures](docu/Data_Structures.html)  
- [Operation Modes](docu/Operation_Modes.html)  
- [Inactivity Handling](docu/Inactivity_Handling.html)  
- [Examples](docu/Examples.html)
- [Best Practices](docu/BestPractices.html)  
- [Troubleshooting Guide](docu/Troubleshooting.html) 

---
 
## Basic Usage

Include the library, create an instance for your radar, and start it with `begin()`.  
Register a callback to get notified when new detection data arrives.

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
