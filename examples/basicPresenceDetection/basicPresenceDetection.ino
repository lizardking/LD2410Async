/**
 * Example: Basic Presence Detection with LD2410Async
 *
 * This sketch demonstrates how to use the LD2410Async library to detect presence
 * using only the presenceDetected variable from the detection data callback.
 * It prints a message with a timestamp whenever the presence state changes.
 *
 * Important!
 * Adjust RADAR_RX_PIN and RADAR_TX_PIN to match your wiring.
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

// Track last presence state
bool lastPresenceDetected = false;
bool firstCallback = true;

// Callback function called whenever new detection data arrives
void onDetectionDataReceived(LD2410Async* sender, bool presenceDetected, byte userData) {
    if (firstCallback || presenceDetected != lastPresenceDetected) {
        unsigned long now = millis();
        Serial.print("[");
        Serial.print(now);
        Serial.print(" ms] Presence detected: ");
        Serial.println(presenceDetected ? "YES" : "NO");
        lastPresenceDetected = presenceDetected;
        firstCallback = false;
    }
}

void setup() {
    // Initialize USB serial for debug output
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for Serial Monitor
    }
    Serial.println("LD2410Async example: basic presence detection");

    // Initialize Serial1 with user-defined pins and baudrate
    RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

    // Start the radar background task (parses incoming data frames)
    if (radar.begin()) {
        Serial.println("Radar task started successfully.");
        // Register callback for detection updates
        radar.registerDetectionDataReceivedCallback(onDetectionDataReceived, 0);
    }
    else {
        Serial.println("ERROR! Could not start radar task.");
    }
}

void loop() {
    // Nothing to do here!
    delay(1000); // idle delay (optional)
}