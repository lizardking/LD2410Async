/**
 * @file tortureRequestCommands.ino
 * @brief
 * LD2410Async torture test: endlessly executes random *request* commands,
 * measures execution time, tracks stats, and prints a summary every 10 minutes
 * or when the spacebar is pressed in the terminal window (will not work in a 
 * purely monitor window).
 *
 * Important!
 * Adjust RADAR_RX_PIN and RADAR_TX_PIN to your wiring.
 */

#include <Arduino.h>
#include <Ticker.h>
#include "LD2410Async.h"

 // ========================= USER CONFIGURATION =========================
 // UART pins for the LD2410 sensor (ESP32 UART1 example)
#define RADAR_RX_PIN 32  // ESP32 pin that receives data from the radar (radar TX)
#define RADAR_TX_PIN 33  // ESP32 pin that transmits data to the radar (radar RX)

// UART baudrate for the radar sensor (factory default is often 256000)
#define RADAR_BAUDRATE 256000

#define LED_PIN 2			  //GPIO for internal LED (adjust if needed)
// =====================================================================

// Create a HardwareSerial instance bound to UART1
HardwareSerial RadarSerial(1);

// Create the LD2410Async instance
LD2410Async ld2410(RadarSerial);

// --------------------------- Stats ----------------------------
//@cond Hid_this_from_the_docu
struct CommandStats {
	const char* name;
	unsigned long minMs;
	unsigned long maxMs;
	unsigned long totalMs;
	unsigned long runs;
	unsigned long failures;  // any non-SUCCESS
	unsigned long timeouts;
	unsigned long failed;
	unsigned long canceled;
};
// @endcond

enum RequestCommand : uint8_t {
	REQ_GATE_PARAMS = 0,
	REQ_FIRMWARE,
	REQ_BT_MAC,
	REQ_DIST_RES,
	REQ_AUX,
	REQ_ALL_CONFIG,
	REQ_ALL_STATIC,
	REQ_COUNT
};

CommandStats stats[REQ_COUNT] = {
  { "requestGateParametersAsync",     ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
  { "requestFirmwareAsync",           ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
  { "requestBluetoothMacAddressAsync",ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
  { "requestDistanceResolutionAsync", ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
  { "requestAuxControlSettingsAsync", ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
  { "requestAllConfigSettingsAsync",  ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
  { "requestAllStaticDataAsync",      ULONG_MAX, 0, 0, 0, 0, 0, 0, 0 },
};

volatile bool commandInFlight = false;
RequestCommand currentCmd = REQ_GATE_PARAMS;
unsigned long cmdStartMs = 0;

// We use a Ticker to retry quickly when a send is rejected (busy)
Ticker retryTicker;

//Just to delay the next command a bit
Ticker delayNextCommandTicker;

// Stats printing
unsigned long lastStatsPrintMs = 0;
const unsigned long STATS_PRINT_PERIOD_MS = 300000;

// ----------------------- Forward decls ------------------------
void runRandomCommand();
void commandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result);

// ------------------------ Helpers ----------------------------
void recordResult(RequestCommand cmd, LD2410Async::AsyncCommandResult result) {
	const unsigned long dur = millis() - cmdStartMs;
	CommandStats& s = stats[cmd];

	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		if (dur < s.minMs) s.minMs = dur;
		if (dur > s.maxMs) s.maxMs = dur;
		s.totalMs += dur;
		s.runs++;
	}
	else {
		s.failures++;
		switch (result) {
		case LD2410Async::AsyncCommandResult::TIMEOUT:  s.timeouts++; break;
		case LD2410Async::AsyncCommandResult::FAILED:   s.failed++;   break;
		case LD2410Async::AsyncCommandResult::CANCELED: s.canceled++; break;
		default: break;
		}
	}
}

void printStats() {
	Serial.println();
	Serial.println("===== LD2410Async Torture Test (REQUEST COMMANDS) =====");

	unsigned long totalRuns = 0;
	unsigned long totalFailures = 0;
	unsigned long totalTimeouts = 0;
	unsigned long totalFailed = 0;
	unsigned long totalCanceled = 0;

	for (int i = 0; i < REQ_COUNT; i++) {
		const CommandStats& s = stats[i];
		const unsigned long minShown = (s.minMs == ULONG_MAX) ? 0 : s.minMs;
		const unsigned long avg = (s.runs > 0) ? (s.totalMs / s.runs) : 0;

		Serial.printf("%-32s | runs: %lu | failures: %lu", s.name, s.runs, s.failures);
		Serial.printf(" (timeouts:%lu failed:%lu canceled:%lu)", s.timeouts, s.failed, s.canceled);
		Serial.printf(" | min:%lums max:%lums avg:%lums\n", minShown, s.maxMs, avg);

		// accumulate totals
		totalRuns += s.runs;
		totalFailures += s.failures;
		totalTimeouts += s.timeouts;
		totalFailed += s.failed;
		totalCanceled += s.canceled;
	}

	// minutes since program start
	unsigned long minutes = millis() / 60000;

	Serial.println("-------------------------------------------------------");
	Serial.printf("TOTALS                          | runs: %lu | failures: %lu",
		totalRuns, totalFailures);
	Serial.printf(" (timeouts:%lu failed:%lu canceled:%lu)\n",
		totalTimeouts, totalFailed, totalCanceled);
	Serial.printf("Uptime: %lu minute(s)\n", minutes);
	Serial.println("=======================================================");
}


void scheduleRetry() {
	// Retry very soon without blocking
	retryTicker.once_ms(1, []() { runRandomCommand(); });
}

// --------------------- Command driver ------------------------
bool sendCommand(RequestCommand cmd) {
	// Start timing *only* when we are about to send.
	digitalWrite(LED_PIN, HIGH);

	cmdStartMs = millis();
	bool accepted = false;

	switch (cmd) {
	case REQ_GATE_PARAMS:
		accepted = ld2410.requestGateParametersAsync(commandCallback);
		break;
	case REQ_FIRMWARE:
		accepted = ld2410.requestFirmwareAsync(commandCallback);
		break;
	case REQ_BT_MAC:
		accepted = ld2410.requestBluetoothMacAddressAsync(commandCallback);
		break;
	case REQ_DIST_RES:
		accepted = ld2410.requestDistanceResolutionAsync(commandCallback);
		break;
	case REQ_AUX:
		accepted = ld2410.requestAuxControlSettingsAsync(commandCallback);
		break;
	case REQ_ALL_CONFIG:
		accepted = ld2410.requestAllConfigSettingsAsync(commandCallback);
		break;
	case REQ_ALL_STATIC:
		accepted = ld2410.requestAllStaticDataAsync(commandCallback);
		break;
	default:
		accepted = false;
		break;
	}

	return accepted;
}

void disableConfigModeCallback(LD2410Async* /*sender*/, LD2410Async::AsyncCommandResult result) {


	runRandomCommand();
}

void runRandomCommand() {
	if (commandInFlight) return; // safety

	if (ld2410.isConfigModeEnabled()) {
		//Need to diable config mode first
		Serial.print("!");
		if (ld2410.disableConfigModeAsync(disableConfigModeCallback)) {
			return;
		}
	}


	currentCmd = static_cast<RequestCommand>(random(REQ_COUNT));
	commandInFlight = true;


	if (!sendCommand(currentCmd)) {
		// Library said "no" (likely busy) — back off and try again
		commandInFlight = false;
		scheduleRetry();
	}
}

void runRandomCommandAfterDelay() {
	switch (random(4)) {
	case 0:
		runRandomCommand();
		break;
	case 1:
		delayNextCommandTicker.once_ms(random(10) + 1, runRandomCommand);
		break;
	case 2:
		delayNextCommandTicker.once_ms((random(10) + 1) * 10, runRandomCommand);
		break;
	default:
		delayNextCommandTicker.once_ms((random(30) + 1) * 100, runRandomCommand);
		break;
	}
}


void commandCallback(LD2410Async* /*sender*/, LD2410Async::AsyncCommandResult result) {
	digitalWrite(LED_PIN, LOW);
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		Serial.print(".");
	}
	else {
		Serial.print("x");
	}
	
	recordResult(currentCmd, result);
	commandInFlight = false;


	runRandomCommandAfterDelay();
		
}

// --------------------- Arduino lifecycle ---------------------
void setup() {
	Serial.begin(115200);
	delay(500);
	Serial.println("LD2410Async torture test starting...");


	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	// Setup UART to the radar
	RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

	// Start LD2410Async background task (required for async operation)
	if (!ld2410.begin()) {
		Serial.println("ERROR: ld2410.begin() failed (already running?)");
	}



	// Kick off the first command
	runRandomCommand();

	lastStatsPrintMs = millis();
}

void loop() {
	// Print stats every 10 minutes
	const unsigned long now = millis();
	if (now - lastStatsPrintMs >= STATS_PRINT_PERIOD_MS) {
		printStats();
		lastStatsPrintMs = now;
	}

	// Print stats immediately if user presses spacebar
	if (Serial.available()) {
		int c = Serial.read();
		if (c == ' ') {
			printStats();
		}
	}


	// Nothing else to do: LD2410Async runs its own FreeRTOS task after begin()
	// Keep loop() lean to maximize timing accuracy for callbacks.
}
