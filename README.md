# LD2410Async – Asynchronous ESP32 Arduino Library for the LD2410 mmWave Radar Sensor

## Introduction

The **LD2410Async** library provides a asynchronous interface for the **Hi-Link LD2410** human presence radar sensor on Arduino/ESP32 platforms.  

LD2410Async runs in the background using FreeRTOS tasks. This allows your main loop to remain responsive while the library continuously processes radar data, handles configuration, and manages timeouts.  

The library offers:  

- **Non-blocking operation** – sensor data is parsed in the background, no polling loops required.  
- **Async command API** – send commands (e.g., request firmware, change settings) without freezing your application.  
- **Configurable callbacks** – get notified when detection data or configuration updates are received.  
- **Presence detection made simple** – the dedicated detection callback provides a quick `presenceDetected` flag without needing to parse the full data set.  
- **Full access to sensor data** – motion, stationary detection, distances, gate signals, engineering data, and more.  

This makes it ideal for applications such as smart lighting, security, automation, and energy-saving systems, where fast and reliable presence detection is essential.  

---

## Installation

You can install this library in two ways:

### 1. Using Arduino Library Manager (recommended)
1. Open the Arduino IDE.  
2. Go to **Tools → Manage Libraries…**.  
3. Search for **LD2410Async**.  
4. Click **Install**.  

### 2. Manual installation
1. Download this repository as a ZIP file.  
2. In the Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library…**, or  
   unzip the file manually into your Arduino `libraries` folder:  
   - Windows: `Documents/Arduino/libraries/`  
   - Linux/macOS: `~/Arduino/libraries/`  
3. Restart the Arduino IDE.  

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
  radar.registerDetectionDataReceivedCallback(onDetectionData, 0);
}

void loop() {
  // Your main loop stays free for other tasks
  delay(1000);
  Serial.println("Main loop running...");
}
```

### Configuration Commands

To request or update configuration, use the async API. For example:

```cpp
// Config data callback function
void onConfigData(LD2410Async* sender, AsyncCommandResult result, byte userData) {
  if (result == AsyncCommandResult::SUCCESS) {
    Serial.println("Config data received.");
    auto config = sender->getConfigData();
    Serial.print("Number of gates: ");
    Serial.println(config.numberOfGates);
  } else {
    Serial.println("Failed to read config.");
  }
}

void someFunction() {
  // Request all configuration parameters from the radar
  radar.requestAllConfigData(onConfigData, 0);
}
```

### More examples

Well-commented example sketches can be found in the examples folder of the library.

---

## Important Notes & Best Practices

When working with the LD2410Async library, keep the following points in mind:

- **Config Mode Handling**  
  - The sensor must be in *config mode* to change settings.  
  - Most async config methods in this library automatically enable and disable config mode for you.  
  - Avoid leaving the sensor in config mode longer than necessary, as this may affect normal operation.

- **Engineering Mode**  
  - Engineering mode provides detailed gate signal data for development and debugging.  
  - Do **not** enable engineering mode in production unless required — it increases the amount of data sent and will trigger the detection callback more often.  
  - Leaving it on continuously can increase CPU load and reduce performance.

  **Keep Callbacks Short**  
  - Callbacks are executed inside the radar’s processing task.  
  - Keep them **short and non-blocking** (e.g., update a variable or post to a queue).  
  - Avoid long delays, heavy computations, or blocking I/O inside callbacks — otherwise, sensor data processing may be delayed.

- **Presence Detection**  
  - Use the dedicated detection callback with the `presenceDetected` flag if you only care about *whether* something is present.  
  - For advanced scenarios (distances, signals, engineering mode), you can access the full `DetectionData` via `getDetectionData()`.

- **Task Management**  
  - The library runs a FreeRTOS background task for receiving and processing data.  
  - Use `begin()` to start it and `end()` to stop it gracefully.  
  - Do not block the main loop for long periods; the radar’s task will continue to run, but your application logic may lag.

- **Inactivity Handling**  
  - The library can automatically reboot the sensor if no activity is detected for a configured period.  
  - You can enable or disable this behavior with `setInactivityHandling(bool enable)` depending on your use case.
  - 
- **Async Command Busy State**  
  - The library ensures only one async command or sequence runs at a time.  
  - Check `asyncIsBusy()` before sending a new command if you are chaining multiple operations.

- **Config Memory Wear**  
  - The LD2410 stores configuration in internal non-volatile memory.  
  - Repeatedly writing unnecessary config updates can wear out the memory over time.  
  - Only send config changes when values actually need to be updated.

Following these practices will help you get stable, reliable operation from the LD2410 sensor while extending its lifetime.

## Troubleshooting

If you run into issues when using the library, check the following common problems:

- **No data received**  
  - Make sure the radar is connected to the correct serial port and pins.  
  - Verify the baud rate: the LD2410 uses `256000` by default.  
  - Ensure `radar.begin()` was called after initializing the serial port.  

- **Callbacks not firing**  
  - Confirm that you registered the callback before expecting data.  
  - Check that the sensor is not stuck in config mode — in config mode, no detection data is sent.  
  - If you enabled engineering mode, expect more frequent callbacks with more data.  

- **Async commands not working**  
  - Only one async command can be active at a time. Use `asyncIsBusy()` to check before sending a new one and/or check the return value of the async command (true indicates that the command has been sent, false indicates that another async command is pending or that a para is invalid).  
  - Some commands require the sensor to be in config mode. Most methods handle this automatically, but if you manually enable config mode, remember to disable it afterward.  

- **Configuration not saved**  
  - Make sure the sensor has finished applying the configuration.  
  - Don’t power off or reset immediately after sending a config command.  
  - Avoid sending repeated unnecessary config updates to prevent memory wear.  

- **Unexpected reboots of Sensor**  
  - Check if inactivity handling is enabled (`setInactivityHandling(true)`).  
  - If enabled, the library may reboot the sensor automatically after a long period without activity.  

- **Main loop blocked or slow**  
  - Review your callback functions. Keep them short and avoid heavy work.  
  - If you need to do more complex processing, copy the data in the callback and handle it later in your main loop or another task.  

- **Strange or invalid data values**  
  - This can happen if the sensor is still in config mode. Ensure it is in normal detection mode.  
  - If you are experimenting with engineering mode, note that it sends extra raw data which may appear unusual if not parsed correctly.

## Debugging Tips

The library includes optional debug output to help you see what is happening inside.  
This is controlled by preprocessor defines that you can enable **before including** the library header in your sketch.

### Enabling Debug Output

- Add one or more of the following `#define` statements at the very top of your main `.ino` file (before any `#include`):  

```cpp
#define ENABLE_DEBUG          // General debug output (mostly on config data and commands)
#define ENABLE_DEBUG_DATA     // Extra output for received data frames

#include <LD2410Async.h>
```

- When enabled, the library will print debug messages to the default Serial port at runtime.

- You can use this to verify that commands are sent, acknowledgements are received, and data frames are being processed correctly.

### Notes

- Debug output is only for troubleshooting. It should **normally be disabled** in production builds for best performance.

- The debug macros are internal to the library. They are not meant to be used in your own code.

- If you need diagnostic output in your own project, use standard Serial.print() calls instead.

By selectively enabling these flags, you can get detailed insight into the communication with the LD2410 sensor whenever you need to troubleshoot.


## License

This library is released under the **MIT License**.  
You are free to use, modify, and distribute it, including for commercial purposes, with no restrictions.  

See the [LICENSE](LICENSE) file for details.
