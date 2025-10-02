/**
 * @brief Example: Changing detection resolution on LD2410
 *
 * @details
 * This sketch demonstrates how to:
 *   1. Initialize the radar on Serial1.
 *   2. Query all current configuration values from the sensor.
 *   3. Clone the config into a local variable.
 *   4. Modify the distance resolution.
 *   5. Apply the new config.
 *   6. Reboot the sensor to ensure changes take effect.
 *
 * @warning
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

/**
* Create a HardwareSerial instance (ESP32 has multiple UARTs) bound to UART1
*/
HardwareSerial RadarSerial(1);

/**
* @brief Creates LD2410Async object bound to the serial port defined in RadarSerial
*/
LD2410Async radar(RadarSerial);

/**
* @brief Callback after the reboot command has been confirmed
*
* @details
* This method just checks and prints the result
*/
void onReboot(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		Serial.println("Radar reboot initiated.");
	}
	else {
		Serial.println("Failed to init reboot.");
	}
}

/**
* @brief Callback after applying modified config
* 
* @details
* After checking if the async configureAllConfigSettingsAsync() method was successful (always do that!), it will initiate a
* reboot of the sensor to activate the changed distance resolution.
* 
* @note
* Always check the result of the async command that has triggered the callback. Otherweise async commands can fail, timeout or get canceled without you relizing it.
* In callback methods is is generally advised to access members of the LD2410Async instance via the sender pointer.
* This ensures that you are allways working with the correct instance, which is important if you have multiple LD2410Async instances.
* Also keep in mind that callbacks should be as short and efficient as possible to avoid blocking the background task of the library.
*/ 
void onConfigApplied(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		Serial.println("Config applied successfully. Rebooting radar...");
		if (!sender->rebootAsync(onReboot)) {
			//It is good practive to check the return value of commands for true/false.
			//False indicates that the command could not be sent (e.g. because another async command is still pending)
			Serial.println("Error! Could not send reboot command to the sensor");
		};
		//alternatively you could also call the LD2410Async instance directly instead of using the pointer in sender:
		//radar.rebootAsync(onReboot);
		//However, working with the reference in sender is a good practice since it will always point to the LD2410Async instance that triggered the callback (important if you have several instances resp. more than one LD2410 connected to your board)..
	}
	else {
		Serial.println("Failed to apply config.");
	}
}

/**
* @brief Callback after requesting config data 
*
* @details
* This method is called when the config data has been received from the sensor. 
* After checking if the async requestAllConfigSettingsAsync() method was successful (always do that!), it clones the config using getConfigData(), 
* modifies the distance resolution and applies the new config using configureAllConfigSettingsAsync()
* 
* @note
* Always check the result of the async command that has triggered the callback. Otherweise async commands can fail, timeout or get canceled without you relizing it.
* In callback methods is is generally advised to access members of the LD2410Async instance via the sender pointer.
* This ensures that you are allways working with the correct instance, which is important if you have multiple LD2410Async instances.
* Also keep in mind that callbacks should be as short and efficient as possible to avoid blocking the background task of the library.
*/
void onConfigReceived(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		Serial.println("Failed to request config data.");
		return;
	}

	Serial.println("Config data received. Cloning and modifying...");

	// Clone the current config
	LD2410Types::ConfigData cfg = sender->getConfigData();

	// Example modification: change resolution
	// RESOLUTION_75CM or RESOLUTION_20CM
	cfg.distanceResolution = LD2410Types::DistanceResolution::RESOLUTION_20CM;

	// Apply updated config to the radar
	if (!sender->configureAllConfigSettingsAsync(cfg, false, onConfigApplied)) {
		//It is good practive to check the return value of commands for true/false.
		//False indicates that the command could not be sent (e.g. because another async command is still pending)
		Serial.println("Error! Could not update config on the sensor");
	};
	//alternatively you could also call the LD2410Async instance directly instead of using the pointer in sender:
	//radar.configureAllConfigSettingsAsync(cfg, false, onConfigApplied);
	//However, working with the reference in sender is a good practice since it will always point to the LD2410Async instance that triggered the callback (important if you have several instances resp. more than one LD2410 connected to your board)..
}

/**
* @brief Arduino setup function which initializes the radar and starts the config change process
* 
* @details
* begin() starts the background task of the LD2410Async library which automatically handles
* incoming data and triggers callbacks. The onDetectionDataReceived callback is registered to 
* receive detection data.
* requestAllConfigSettingsAsync() will fetch all config data from the sensor and triggers the
* onConfigReceived callback when done. 
*/
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

		// Request all config data, then modify in callback
		if (!radar.requestAllConfigSettingsAsync(onConfigReceived)) {
			//It is good practive to check the return value of commands for true/false.
			//False indicates that the command could not be sent (e.g. because another async command is still pending)
			Serial.println("Error! Could not start config data request");
		}
	}
	else {
		Serial.println("Error! Could not start radar task.");
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
	// Nothing to do here.
	// The library handles communication and invokes callbacks automatically.
	delay(1000);
}
