/**
 * Example sketch for LD2410Async library
 *
 * This sketch demonstrates how to:
 *   1. Initialize the radar on Serial1.
 *   2. Query all current configuration values from the sensor.
 *   3. Clone the config into a local variable.
 *   4. Modify the distance resolution.
 *   5. Apply the new config.
 *   6. Reboot the sensor to ensure changes take effect.
 *
 * You can change the RX/TX pins at the top of the file to match your wiring.
 */

#include <Arduino.h>
#include "LD2410Async.h"

 // ========================= USER CONFIGURATION =========================

 // UART pins for the LD2410 sensor
#define RADAR_RX_PIN 16   // ESP32 pin that receives data from the radar (radar TX)
#define RADAR_TX_PIN 17   // ESP32 pin that transmits data to the radar (radar RX)

// UART baudrate for the radar sensor (default is 256000)
#define RADAR_BAUDRATE 256000

// ======================================================================

// Create a HardwareSerial instance (ESP32 has multiple UARTs)
HardwareSerial RadarSerial(1);

// Create LD2410Async object bound to Serial1
LD2410Async radar(RadarSerial);

// Callback after reboot command is sent
void onReboot(LD2410Async* sender,
    LD2410Async::AsyncCommandResult result,
    byte userData) {
    if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
        Serial.println("Radar reboot initiated.");
    }
    else {
        Serial.println("Failed to init reboot.");
    }
}

// Callback after applying modified config
void onConfigApplied(LD2410Async* sender,
    LD2410Async::AsyncCommandResult result,
    byte userData) {
    if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
        Serial.println("Config applied successfully. Rebooting radar...");
        sender->rebootAsync(onReboot);
    }
    else {
        Serial.println("Failed to apply config.");
    }
}

// Callback after requesting config data
void onConfigReceived(LD2410Async* sender,
    LD2410Async::AsyncCommandResult result,
    byte userData) {
    if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
        Serial.println("Failed to request config data.");
        return;
    }

    Serial.println("Config data received. Cloning and modifying...");

    // Clone the current config
    LD2410Async::ConfigData cfg = sender->getConfigData();

    // Example modification: change resolution
    // RESOLUTION_75CM or RESOLUTION_20CM
    cfg.distanceResolution = LD2410Async::DistanceResolution::RESOLUTION_20CM;

    // Apply updated config
    sender->setConfigDataAsync(cfg, onConfigApplied);
}

void setup() {
    // Initialize USB serial for debug output
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for Serial Monitor
    }
    Serial.println("LD2410Async example: change resolution and reboot");

    // Initialize Serial1 with user-defined pins and baudrate
    RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

    // Start the radar background task (parses incoming data frames)
    if (radar.begin()) {
        Serial.println("Radar task started successfully.");
    }
    else {
        Serial.println("Radar task already running.");
    }

    // Request all config data, then modify in callback
    radar.requestAllConfigData(onConfigReceived);
}

void loop() {
    // Nothing to do here.
    // The library handles communication and invokes callbacks automatically.
    delay(1000);
}
