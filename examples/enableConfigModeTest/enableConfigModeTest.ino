/**
 * @file measureEnableConfigMode.ino
 * @brief
 * Test sketch for LD2410Async library to evaluate the execution time
 * of the enableConfigModeAsync() command.
 *
 * @details
 * This sketch continuously executes a measurement sequence:
 *  - Calls enableConfigModeAsync() and measures how long it takes until
 *    the ACK (callback) is triggered.
 *  - Once config mode is enabled, executes one randomly chosen command
 *    (requestDistanceResolutionAsync, requestAuxControlSettingsAsync,
 *    requestAllConfigSettingsAsync, requestAllStaticDataAsync,
 *    requestFirmwareAsync, or no command at all).
 *  - Calls disableConfigModeAsync().
 *  - Waits a random timespan between 1–5000 ms using the Ticker library.
 *  - Repeats the sequence indefinitely.
 *
 * The sketch prints the minimum, maximum, average, and count of all
 * measurements after each test cycle.
 */

#include <Arduino.h>
#include <Ticker.h>
#include "LD2410Async.h"

 // ========================= USER CONFIGURATION =========================

#define RADAR_RX_PIN 32       ///< ESP32 pin receiving data from the radar (radar TX)
#define RADAR_TX_PIN 33       ///< ESP32 pin transmitting data to the radar (radar RX)
#define RADAR_BAUDRATE 256000 ///< UART baudrate for radar sensor (default is 256000)
#define LED_PIN 2			  ///< GPIO for internal LED (adjust if needed)
// ======================================================================
// Global objects
// ======================================================================

HardwareSerial RadarSerial(1);   ///< HardwareSerial instance bound to UART1
LD2410Async ld2410(RadarSerial); ///< LD2410Async driver instance
Ticker delayTicker;              ///< Ticker for randomized delays

// ======================================================================
// Measurement variables
// ======================================================================

unsigned long enableStartMs = 0;           ///< Timestamp when enableConfigModeAsync() was called
unsigned long minEnableDurationMs = ULONG_MAX; ///< Minimum execution time measured so far
unsigned long maxEnableDurationMs = 0;     ///< Maximum execution time measured so far
unsigned long totalEnableDurationMs = 0;   ///< Sum of all measured durations
unsigned long measurementCount = 0;        ///< Number of successful measurements
unsigned long failureCount = 0;            ///< Number of failed measurements
unsigned long delayMs = 0;				   ///< Delay before the tests
unsigned long dataCount = 0;			   ///< Used to count received data frames
// ======================================================================
// Forward declarations of callbacks
// ======================================================================

void enableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result);
void extraCommandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result);
void disableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result);


// ======================================================================
// Utility functions
// ======================================================================

/**
 * @brief Print a number padded to 6 characters width.
 *
 * @param value Number to print
 */
void printPaddedNumber(unsigned long value) {
	char buf[8];
	snprintf(buf, sizeof(buf), "%6lu", value);
	Serial.print(buf);
}

/**
 * @brief Convert AsyncCommandResult enum to human-readable text.
 *
 * @param result Enum value from LD2410Async
 * @return const char* String representation
 */
const char* resultToString(LD2410Async::AsyncCommandResult result) {
	switch (result) {
	case LD2410Async::AsyncCommandResult::SUCCESS:  return "SUCCESS";
	case LD2410Async::AsyncCommandResult::FAILED:   return "FAILED";
	case LD2410Async::AsyncCommandResult::TIMEOUT:  return "TIMEOUT";
	case LD2410Async::AsyncCommandResult::CANCELED: return "CANCELED";
	default: return "UNKNOWN";
	}
}

// ======================================================================
// Test sequence functions
// ======================================================================

/**
 * @brief Schedule the next test sequence.
 *
 * Waits a random time between 1–5000 ms before calling enableConfigModeAsync().
 */
void scheduleNextSequence() {
	dataCount = 0;

	switch (random(0, 4)) {
	case 0:
		delayMs = random(1, 11);
		break;
	case 1:
		delayMs = random(1, 11) * 10;;
		break;
	case 2:
		delayMs = random(0, 50) * 20 + 100;
		break;
	default:
		delayMs = random(0, 70) * 100 + 1000;
		break;
	}

	measurementCount++;

	Serial.print("Test ");
	printPaddedNumber(measurementCount);
	Serial.print(" | Delay: ");
	printPaddedNumber(delayMs);


	delayTicker.once_ms(delayMs, []() {
		Serial.print(" (Data cnt: ");
		printPaddedNumber(dataCount);
		Serial.print(")");
		enableStartMs = millis();
		dataCount = 0;
		if (!ld2410.enableConfigModeAsync(enableConfigModeCallback)) {
			Serial.println(" > enableConfigModeAsync could not be started (busy).");
			failureCount++;
			scheduleNextSequence();
		}
		else {
			Serial.print(" > ");
		}
	});
}

/**
 * @brief Callback for enableConfigModeAsync().
 *
 * Updates statistics, prints results and triggers an extra command.
 *
 * @param sender Pointer to the LD2410Async instance
 * @param result Result of the command
 */
void enableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	unsigned long duration = millis() - enableStartMs;

	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		totalEnableDurationMs += duration;
		if (duration > maxEnableDurationMs) maxEnableDurationMs = duration;
		if (duration < minEnableDurationMs) minEnableDurationMs = duration;

		unsigned long avg = totalEnableDurationMs / measurementCount;


		Serial.print("Dur: ");
		printPaddedNumber(duration);
		Serial.print(" ms | Data Cnt:");
		printPaddedNumber(dataCount);
		Serial.print(" | Min: ");
		printPaddedNumber(minEnableDurationMs);
		Serial.print(" ms | Max: ");
		printPaddedNumber(maxEnableDurationMs);
		Serial.print(" ms | Avg: ");
		printPaddedNumber(avg);
		Serial.print(" ms | Fails: ");
		printPaddedNumber(failureCount);
		Serial.println();

		// Execute a random extra command
		int cmd = random(0, 6);
		switch (cmd) {
		case 0: ld2410.requestDistanceResolutionAsync(extraCommandCallback); break;
		case 1: ld2410.requestAuxControlSettingsAsync(extraCommandCallback); break;
		case 2: ld2410.requestAllConfigSettingsAsync(extraCommandCallback); break;
		case 3: ld2410.requestAllStaticDataAsync(extraCommandCallback); break;
		case 4: ld2410.requestFirmwareAsync(extraCommandCallback); break;
		case 5: // No extra command
			if (!ld2410.disableConfigModeAsync(disableConfigModeCallback)) {
				Serial.println("Could not start disableConfigMode()");
				scheduleNextSequence();
			}
			break;
		}
	}
	else {
		Serial.print("enableConfigModeAsync failed: ");
		Serial.print(resultToString(result));
		Serial.print(" Received ");
		printPaddedNumber(dataCount);
		Serial.println(" data frames while waiting for ACK");
		failureCount++;
		measurementCount--;
		scheduleNextSequence();
	}
}

/**
 * @brief Callback for extra commands.
 *
 * @param sender Pointer to the LD2410Async instance
 * @param result Result of the command
 */
void extraCommandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		Serial.print("Extra command failed ");
		Serial.println(resultToString(result));
	}
	if (!ld2410.disableConfigModeAsync(disableConfigModeCallback)) {
		Serial.println("Could not start disableConfigMode()");
		scheduleNextSequence();
	}
}

/**
 * @brief Callback for disableConfigModeAsync().
 *
 * Once config mode is disabled, schedules the next test sequence.
 *
 * @param sender Pointer to the LD2410Async instance
 * @param result Result of the command
 */
void disableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		Serial.print("Disable config mode failed ");
		Serial.println(resultToString(result));
	}
	scheduleNextSequence();
}

// ======================================================================
// Callback function
// ======================================================================

/**
 * @brief Callback triggered when new detection data is received.
 *
 * @param sender Pointer to the LD2410Async instance
 * @param presenceDetected Convenience flag for presence detection
 */
void onDetectionDataReceived(LD2410Async* sender, bool presenceDetected) {
	// Toggle LED whenever data arrives
	int currentState = digitalRead(LED_PIN);
	digitalWrite(LED_PIN, currentState == HIGH ? LOW : HIGH);

	//Count data frames
	dataCount++;
}
// ======================================================================
// Setup and loop
// ======================================================================

/**
 * @brief Arduino setup function.
 *
 * Initializes Serial, the radar UART, and the LD2410Async library.
 * Starts the first test sequence by calling scheduleNextSequence().
 */
void setup() {
	Serial.begin(115200);
	delay(2000);
	Serial.println("=== LD2410Async enableConfigModeAsync measurement test ===");

	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

	if (!ld2410.begin()) {
		Serial.println("Failed to start LD2410Async task!");
		while (true) delay(1000);
	}
	else {
		Serial.println("LD2410Async task started!");

		//Callback reghistration can take place at any time
		ld2410.onDetectionDataReceived(onDetectionDataReceived);
	}

	// Long timeout to ensure that we can really measure the actual execution time
	ld2410.setAsyncCommandTimeoutMs(60000);

	// Kick off the very first sequence
	scheduleNextSequence();
}

/**
 * @brief Arduino loop function.
 *
 * Empty because everything is event-driven using callbacks and ticker.
 */
void loop() {
	delay(1000); // idle delay
}
