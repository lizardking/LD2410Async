/**
 * Example sketch for LD2410Async library
 *
 * This sketch initializes the LD2410 radar sensor on Serial1 and
 * prints detection data to the Serial Monitor as soon as it arrives.
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

// Callback function called whenever new detection data arrives
void onDetectionDataReceived(LD2410Async* sender, byte userData) {
    // Access detection data efficiently without making a copy
    const LD2410Async::DetectionData& data = sender->getDetectionDataRef();

    Serial.println("=== Detection Data ===");

    Serial.print("Target State: ");
    Serial.println(LD2410Async::targetStateToString(data.targetState));

    Serial.print("Presence detected: ");
    Serial.println(data.presenceDetected ? "Yes" : "No");

    Serial.print("Moving presence: ");
    Serial.println(data.movingPresenceDetected ? "Yes" : "No");

    Serial.print("Stationary presence: ");
    Serial.println(data.stationaryPresenceDetected ? "Yes" : "No");

    Serial.print("Moving Distance (cm): ");
    Serial.println(data.movingTargetDistance);

    Serial.print("Stationary Distance (cm): ");
    Serial.println(data.stationaryTargetDistance);

    Serial.print("Light Level: ");
    Serial.println(data.lightLevel);

    Serial.println("======================");
}

void setup() {
    // Initialize USB serial for debug output
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for Serial Monitor
    }
    Serial.println("LD2410Async example: print detection data");

    // Initialize Serial1 with user-defined pins and baudrate
    RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

    // Start the radar background task (parses incoming data frames)
    if (radar.begin()) {
        Serial.println("Radar task started successfully.");
    }
    else {
        Serial.println("Radar task already running.");
    }

    // Register callback for detection updates
    radar.registerDetectionDataReceivedCallback(onDetectionDataReceived);
}

void loop() {
    // Nothing to do here!
    // The LD2410Async library runs a FreeRTOS background task
    // that automatically parses incoming radar data and triggers callbacks.
    delay(1000); // idle delay (optional)
}
