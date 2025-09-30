/**
* @file 
* @brief Example: Basic Presence Detection with LD2410Async
*
* This sketch demonstrates how to use the LD2410Async library to detect presence
* using only the presenceDetected variable from the detection data callback.
* It prints a message with a timestamp whenever the presence state changes.
*
* @warning
* Important!
* Adjust RADAR_RX_PIN and RADAR_TX_PIN to match your wiring.
* 
* @ingroup examples 
*/

#include <Arduino.h>
#include "LD2410Async.h"

 // ========================= USER CONFIGURATION =========================
/// @name User Configuration
/// @brief Pins and constants that must be adjusted for your wiring and sensor config.
/// @{

// UART pins for the LD2410 sensor
#define RADAR_RX_PIN 16   // ESP32 pin that receives data from the radar (radar TX)
#define RADAR_TX_PIN 17   // ESP32 pin that transmits data to the radar (radar RX)

// UART baudrate for the radar sensor (default is 256000)
#define RADAR_BAUDRATE 256000
/// @}

// ======================================================================

/**
* Create a HardwareSerial instance (ESP32 has multiple UARTs) bound to UART1
*/
HardwareSerial RadarSerial(1);

/**
* @brief Creates LD2410Async object bound to the serial port defined in RadarSerial
*/
LD2410Async radar(RadarSerial);


// Track last presence state
bool lastPresenceDetected = false;
bool firstCallback = true;

/**
* @brief Callback function called whenever new detection data arrives
*
* @details
* Since only basic presence information is required in the example, it is just using the presenceDetected para of the callback.
* The logic in this methods just ensures that only changes in presence state are printed to the Serial Monitor.
*
 * @note
 * If only basic presence detection is needed, use the presenceDetected variable directly instead of accessing the full detection data struct.
 * For more advanced use cases, the full detection data can be accessed using getDetectionDataRef() or getDetectionData().
*/
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

/**
* @brief Arduino setup function which initializes the radar and registers the callback
*
* @details
* radar.begin() starts the background task of the LD2410Async library which automatically handles
* incoming data and triggers callbacks. The onDetectionDataReceived callback is registered to receive detection data.
*
*/
void setup() {
    // Initialize USB serial for debug output
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for Serial Monitor
    }
    Serial.println("LD2410Async Example: Basic Presence Detection");

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

/**
* @brief Arduino loop function which does nothing
*
* @details
* The LD2410Async library runs a FreeRTOS background task that automatically handles all jobs that are related to the radar sensor.
* Therefore the main loop doesnt have to da any LD2410 related work and is free for anything else you might want to do.
*/
void loop() {
    // Nothing to do here!
    delay(1000); // idle delay (optional)
}