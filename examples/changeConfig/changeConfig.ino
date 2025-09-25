/**
 * Example sketch for LD2410Async library
 *
 * This sketch shows how to:
 *   1. Initialize the radar on Serial1.
 *   2. Query all current configuration values from the sensor.
 *   3. Clone the config into a local variable.
 *   4. Modify some settings (timeout, sensitivities).
 *   5. Write the modified config back to the sensor.
 *
 * Important:
 * Make sure to adjust RADAR_RX_PIN and ADAR_TX_PIN to match you actual wiring.
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

// Callback after attempting to apply a modified config
void onConfigApplied(LD2410Async* sender,
    LD2410Async::AsyncCommandResult result,
    byte userData) {
    if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
        Serial.println("Config updated successfully!");
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

    // Example modifications:
    cfg.noOneTimeout = 120;                         // Set "no-one" timeout to 120 seconds
    cfg.distanceGateMotionSensitivity[2] = 75;      // Increase motion sensitivity on gate 2
    cfg.distanceGateStationarySensitivity[4] = 60;  // Adjust stationary sensitivity on gate 4

    // Apply updated config to the radar
    sender->setConfigDataAsync(cfg, onConfigApplied);
}

void setup() {
    // Initialize USB serial for debug output
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for Serial Monitor
    }
    Serial.println("LD2410Async example: change radar config");

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
