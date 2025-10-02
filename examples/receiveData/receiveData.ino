/**
* @brief: Example: Receive detection data from the LD2410
*
* @details
* This sketch initializes the LD2410 radar sensor on Serial1 and
* prints detection data to the Serial Monitor as soon as it arrives.
* This sketch demonstrates how to:
*   1. Initialize the radar on Serial1.
*   2. register a callback to receive detection data.
*   3. Get a pointer to the detection data struct. 
*
* @warning
* Important:
* Make sure to adjust RADAR_RX_PIN and RADAR_TX_PIN to match you actual wiring.
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

/**
* Create a HardwareSerial instance (ESP32 has multiple UARTs) bound to UART1
*/
HardwareSerial RadarSerial(1);

/**
* @brief Creates LD2410Async object bound to the serial port defined in RadarSerial
*/
LD2410Async radar(RadarSerial);

/*
 * @brief Callback function called whenever new detection data arrives
 * 
 * @details
 * The detection data is accessed via a reference to avoid making a copy. This is more efficient and saves memory.
 * The callback provides the presenceDetected variable for convenience, but all other detection data is available 
 * in the DetectionData struct that can be accessed using .getDetectionDataRef()
 * 
 * @note
 * In callback methods is is generally advised to access members of the LD2410Async instance via the sender pointer. 
 * This ensures that you are allways working with the correct instance, which is important if you have multiple LD2410Async instances.
 * Also keep in mind that callbacks should be as short and efficient as possible to avoid blocking the background task of the library.
 */
void onDetectionDataReceived(LD2410Async* sender, bool presenceDetected) {
    // Access detection data efficiently without making a copy
    const LD2410Types::DetectionData& data = sender->getDetectionDataRef();

    Serial.println("=== Detection Data ===");

    Serial.print("Target State: ");
    Serial.println(LD2410Types::targetStateToString(data.targetState));

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
    Serial.println("LD2410Async Example: Receive Data");

    // Initialize Serial1 with user-defined pins and baudrate
    RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

    // Start the radar background task (parses incoming data frames)
    if (radar.begin()) {
        Serial.println("Radar task started successfully.");
        // Register callback for detection updates
        radar.registerDetectionDataReceivedCallback(onDetectionDataReceived);
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
    // The LD2410Async library runs a FreeRTOS background task
    // that automatically parses incoming radar data and triggers callbacks.
    delay(1000); // idle delay (optional)
}
