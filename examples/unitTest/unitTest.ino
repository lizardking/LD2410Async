

#include <Arduino.h>
#include <Ticker.h>

#include "LD2410Async.h"


/********************************************************************************************************************************
** Hardware configuration
********************************************************************************************************************************/
// UART pins for the LD2410 sensor
//#define RADAR_RX_PIN 16   // ESP32 pin that receives data from the radar (radar TX)
//#define RADAR_TX_PIN 17   // ESP32 pin that transmits data to the radar (radar RX)

#define RADAR_RX_PIN 32
#define RADAR_TX_PIN 33


// UART baudrate for the radar sensor (default is 256000)
#define RADAR_BAUDRATE 256000

/********************************************************************************************************************************
** Variables
********************************************************************************************************************************/
// Create a HardwareSerial instance (ESP32 has multiple UARTs)
HardwareSerial RadarSerial(1);

// Create LD2410Async object bound to Serial1
LD2410Async ld2410(RadarSerial);

// Used to track the duration of the tests
#include <Arduino.h>


// Ticker used for delays in the test methods
Ticker onceTicker;

// Config data as found on the sensor, so we can restore it when done.
LD2410Types::ConfigData orgConfigData;

//Used to determine the duration of the tests
unsigned long testStartMs = 0;
//Name of the currently running tests
String currentTestName;

//Counts test outcomes
int testSuccessCount = 0;
int testFailCount = 0;

//Number of test that is currently executed. Is used to iterate through the test sequence
int currentTestIndex = -1;
//Indicates whether tests are currently being executed
bool testSequenceRunning = false;

//Line length for the printed outputs
const int LINE_LEN = 80;

/********************************************************************************************************************************
** Print functions
** They just help to get nice output from the tests
********************************************************************************************************************************/
void printLine(char c, int len) {
	for (int i = 0; i < len; i++) Serial.print(c);
	Serial.println();
}

void printBigMessage(const String msg, char lineChar = '*') {
	Serial.println();
	printLine(lineChar, LINE_LEN);
	printLine(lineChar, LINE_LEN);
	Serial.print(lineChar);
	Serial.println(lineChar);
	Serial.print(lineChar);
	Serial.print(lineChar);
	Serial.print(" ");
	Serial.println(msg);
	Serial.print(lineChar);
	Serial.println(lineChar);
	printLine(lineChar, LINE_LEN);
	printLine(lineChar, LINE_LEN);
}

/********************************************************************************************************************************
** Generic print methods so we can pass parameters with differenrt types
********************************************************************************************************************************/
template <typename T>
void printOne(const T& value) {
	Serial.print(value);
}

template <typename T, typename... Args>
void printOne(const T& first, const Args&... rest) {
	Serial.print(first);
	printOne(rest...); // recurse
}

/********************************************************************************************************************************
** Test start
** Must be called at the start of each test.
** Outputs the test name, plus comment and records the starting time of the test.
********************************************************************************************************************************/
void testStart(String name, String comment = "") {
	currentTestName = name;

	printLine('*', LINE_LEN);
	Serial.print("** ");
	Serial.println(name);
	if (comment.length() > 0) {
		Serial.print("** ");
		printLine('-', LINE_LEN - 3);
		Serial.print("** ");
		Serial.println(comment);
	}
	printLine('*', LINE_LEN);
	testStartMs = millis();
}


/********************************************************************************************************************************
** Test End
** Must be called at the end of each test.
** Outputs the test result, duration and comment.
** Also starts the next test on success or aborts the test sequence on failure.
********************************************************************************************************************************/
template <typename... Args>
void testEnd(bool success, const Args&... commentParts) {


	char lineChar = success ? '-' : '=';

	printLine(lineChar, LINE_LEN);

	Serial.print(lineChar);
	Serial.print(lineChar);
	Serial.print(" ");
	Serial.print(currentTestName);
	Serial.print(": ");
	Serial.println(success ? "Success" : "Failed");

	printLine(lineChar, LINE_LEN);

	Serial.print(lineChar);
	Serial.print(lineChar);
	Serial.print(" Test duration (ms): ");
	Serial.println(millis() - testStartMs);

	if constexpr (sizeof...(commentParts) > 0) {
		Serial.print(lineChar);
		Serial.print(lineChar);
		Serial.print(" ");
		printOne(commentParts...);
		Serial.println();
	}


	printLine(lineChar, LINE_LEN);
	Serial.println();

	// update counters
	if (success) {
		testSuccessCount++;
		startNextTest();
	}
	else {
		testFailCount++;
		testSequenceRunning = false;
		printBigMessage("TESTS FAILED", '-');
	}

}


/********************************************************************************************************************************
** Test print
** Outputs intermadate states and results of the tests
********************************************************************************************************************************/
// Versatile testPrint
template <typename... Args>
void testPrint(const Args&... args) {
	unsigned long elapsed = millis() - testStartMs;
	Serial.print(elapsed);
	Serial.print("ms ");
	printOne(args...);
	Serial.println();
}

/********************************************************************************************************************************
** Helper function to convert async test results into a printable string
********************************************************************************************************************************/
String asyncCommandResultToString(LD2410Async::AsyncCommandResult result) {
	switch (result) {
	case LD2410Async::AsyncCommandResult::SUCCESS:  return "SUCCESS";
	case LD2410Async::AsyncCommandResult::FAILED:   return "FAILED (Command has failed)";
	case LD2410Async::AsyncCommandResult::TIMEOUT:  return "TIMEOUT (Async command has timed out)";
	case LD2410Async::AsyncCommandResult::CANCELED: return "CANCELED (Async command has been canceled by the user)";
	default:                           return "UNKNOWN (Unsupported command result, verify code of lib)";
	}
}

/********************************************************************************************************************************
** Helper function to initalize config data strzucts with valid values
********************************************************************************************************************************/
void initializeConfigDataStruct(LD2410Types::ConfigData& configData)
{
	configData.numberOfGates = 9; // This is a read-only value, but we set it here to avoid validation errors
	configData.lightControl = LD2410Types::LightControl::LIGHT_ABOVE_THRESHOLD;
	configData.outputControl = LD2410Types::OutputControl::DEFAULT_LOW_DETECTED_HIGH;
	configData.distanceResolution = LD2410Types::DistanceResolution::RESOLUTION_75CM;

	for (int i = 0; i < 9; i++) {
		configData.distanceGateMotionSensitivity[i] = 5 + 5 * i;
		configData.distanceGateStationarySensitivity[i] = 100 - i;
	}
	configData.maxMotionDistanceGate = 5;
	configData.maxStationaryDistanceGate = 4;
	configData.lightThreshold = 128;
	configData.noOneTimeout = 120;
}


/********************************************************************************************************************************
** Data update counting
** The following methods are used, to monitor whether the LD2410 sends data
********************************************************************************************************************************/

//Counter variables
int dataUpdateCounter_normalModeCount = 0;
int dataUpdateCounter_engineeringModeCount = 0;

//Resets the counter variables
void dataUpdateCounterReset() {
	dataUpdateCounter_normalModeCount = 0;
	dataUpdateCounter_engineeringModeCount = 0;
}

//Callback method for data received event
void dataUpdateCounterCallback(LD2410Async* sender, bool presenceDetected, byte userData) {
	//Output . or * depending on presence detected
	Serial.print(presenceDetected ? "x" : ".");

	//Record whether normal or engineering mode data has been received
	const LD2410Types::DetectionData& data = ld2410.getDetectionDataRef();
	if (data.engineeringMode) {
		dataUpdateCounter_engineeringModeCount++;
	}
	else {
		dataUpdateCounter_normalModeCount++;
	}
}

//Start the data update counting
void startDataUpdateCounter() {
	dataUpdateCounterReset();
	ld2410.registerDetectionDataReceivedCallback(dataUpdateCounterCallback);
}

//Stop the data update counting
void stopDataUpdateCounter() {
	ld2410.registerDetectionDataReceivedCallback(nullptr);
	Serial.println();
}

/********************************************************************************************************************************
** Begin test
********************************************************************************************************************************/
void beginTest() {

	testStart("begin() Test", "Calling begin() is always the first thing we have to do.");
	if (ld2410.begin()) {
		testEnd(true, "LD2410Async task started successfully.");
	}
	else {
		testEnd(false, "LD2410Async task already running.");
	}
}

/********************************************************************************************************************************
** Reboot test
********************************************************************************************************************************/

void rebootTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for rebootAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true, "LD2410 has rebooted");
	}
	else {
		testEnd(false, "Test has failed due to ", asyncCommandResultToString(asyncResult));
	}
}


void rebootTest() {
	testStart("rebootAsync() Test", "First real test is a reboot of the LD2410 to ensure it is in normal operation mode.");

	bool ret = ld2410.rebootAsync(rebootTestCallback);

	if (ret) {
		testPrint("rebootAsync() conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "rebootAsync() has returned false. This should only happen if another async command is pending pending");
	}
}




/********************************************************************************************************************************
** Normal mode data receive test
********************************************************************************************************************************/

void normalModeDataReceiveTestEnd() {
	stopDataUpdateCounter();
	Serial.println();

	if (dataUpdateCounter_normalModeCount > 0 && dataUpdateCounter_engineeringModeCount == 0) {
		testEnd(true, dataUpdateCounter_normalModeCount, " normal mode data updates received");
	}
	else if (dataUpdateCounter_normalModeCount == 0 && dataUpdateCounter_engineeringModeCount > 0) {
		testPrint("Test failed. No normal mode data updates received, ");
		testPrint("  but got ", dataUpdateCounter_normalModeCount, " engineering mode data updates.");
		testPrint("  Since only engineering mode data was excpected, the test has failed");

		testEnd(false, "Got only engineering mode data instead of normal mode data as expected.");
	}
	else if (dataUpdateCounter_normalModeCount == 0 && dataUpdateCounter_engineeringModeCount == 0) {
		testPrint("Test failed. No data updates received ");
		testEnd(false, "No data updates received");
	}
	else {
		testPrint("Test failed. Received normal mode and engineering mode data,");
		testPrint("  but expected only normal mode data.");
		testEnd(false, "Received a mix of normal mode and engineering mode data, but expected only normal mode data.");

	}
}

void normalModeDataReceiveTest() {
	testStart("Normal Mode Data Receive Test", "Tests whether the sensor sends data in normal mode");

	startDataUpdateCounter();

	testPrint("Callback to get data updates registered with registerDetectionDataReceivedCallback().");
	testPrint("Counting and checking received data for 10 secs");

	onceTicker.once_ms(10000, normalModeDataReceiveTestEnd);

}



/********************************************************************************************************************************
** Enable engineering mode test
********************************************************************************************************************************/

void enableEngineeringModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for enableEngineeringModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true);
	}
	else {
		testEnd(false, "Test has failed due to ", asyncCommandResultToString(asyncResult));
	}
}


void enableEngineeringModeTest() {
	testStart("enableEngineeringModeAsync() Test", "Enables engineering mode, so we get more detailed detection data");

	bool ret = ld2410.enableEngineeringModeAsync(enableEngineeringModeCallback);

	if (ret) {
		testPrint("enableEngineeringModeAsync() conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "enableEngineeringModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}

/********************************************************************************************************************************
** Engineering mode data receive test
********************************************************************************************************************************/

void engineeringModeDataReceiveTestEnd() {
	stopDataUpdateCounter();

	if (dataUpdateCounter_engineeringModeCount > 0 && dataUpdateCounter_normalModeCount == 0) {
		testEnd(true, dataUpdateCounter_engineeringModeCount, " engineering mode data updates received");
	}
	else if (dataUpdateCounter_engineeringModeCount == 0 && dataUpdateCounter_normalModeCount > 0) {
		testPrint("Test failed. No engineering mode data updates received, ");
		testPrint("  but got ", dataUpdateCounter_engineeringModeCount, " normal mode data updates.");
		testPrint("  Since only normal mode data was excpected, the test has failed");

		testEnd(false, "Got only normal mode data instead of engineering mode data as expected.");
	}
	else if (dataUpdateCounter_engineeringModeCount == 0 && dataUpdateCounter_engineeringModeCount == 0) {
		testPrint("Test failed. No data updates received ");
		testEnd(false, "No data updates received");
	}
	else {
		testPrint("Test failed. Received engineering mode and normal mode data,");
		testPrint("  but expected only engineering mode data.");
		testEnd(false, "Received a mix of engineering mode and normal mode data, but expected only engineering mode data.");

	}
}

void engineeringModeDataReceiveTest() {
	testStart("Engineering Mode Data Receive Test", "Tests whether the sensor sends data in engineering mode");

	startDataUpdateCounter();

	testPrint("Callback to get data updates registered with registerDetectionDataReceivedCallback().");
	testPrint("Counting and checking received data for 10 secs");

	onceTicker.once_ms(10000, engineeringModeDataReceiveTestEnd);

}



/********************************************************************************************************************************
** Disable engineering mode test
********************************************************************************************************************************/

void disableEngineeringModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for disableEngineeringModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true);
	}
	else {
		testEnd(false, "Test has failed due to ", asyncCommandResultToString(asyncResult));
	}
}


void disableEngineeringModeTest() {
	testStart("disableEngineeringModeAsync() Test", "Disables engineering mode.");

	bool ret = ld2410.disableEngineeringModeAsync(disableEngineeringModeCallback);

	if (ret) {
		testPrint("disableEngineeringModeAsync() conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "disableEngineeringModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}

/********************************************************************************************************************************
** Enable config mode test
********************************************************************************************************************************/

void enableConfigModeTestEnd() {
	stopDataUpdateCounter();
	if (dataUpdateCounter_engineeringModeCount == 0 && dataUpdateCounter_engineeringModeCount == 0) {
		testEnd(true, "Config mode has been enabled successfully");
	}
	else {
		Serial.println();
		testEnd(false, "enableConfigModeAsync() reports success, but the LD2410 still sends detection data.");
	}
}

void enableConfigModeTestCheckForDataUpdates() {
	testPrint("Waiting a 5 seconds, so we can be sure that we are really in config mode (checking if no data updates are sent).");

	startDataUpdateCounter();

	onceTicker.once_ms(5000, enableConfigModeTestEnd);
}


void enableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for enableConfigModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		enableConfigModeTestCheckForDataUpdates();
	}
	else {
		testEnd(false, "Test has failed due to ", asyncCommandResultToString(asyncResult));
	}
}


void enableConfigModeTest() {
	testStart("enableConfigModeAsync() Test", "Enables config mode, which is required if other commands have to be sent to the LD2410.");

	bool ret = ld2410.enableConfigModeAsync(enableConfigModeCallback);

	if (ret) {
		testPrint("enableConfigModeAsync() conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "enableConfigModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}


/********************************************************************************************************************************
** Config mode persistence test
********************************************************************************************************************************/

void configModePersistenceTestEnd() {
	stopDataUpdateCounter();
	if (dataUpdateCounter_engineeringModeCount == 0 && dataUpdateCounter_normalModeCount == 0) {
		testEnd(true, "Config mode is still active as expected");
	}
	else {
		testEnd(false, "Config mode has been disabled unexpectedly (we got data updates)");
	}
}

void configModePersistenceTestCheckConfigMode() {
	if (ld2410.isConfigModeEnabled()) {
		testPrint("Config mode is still active according to isConfigModeEnabled().");
		testPrint("Wait 5 secs to check if we receive any detection data (which would indicate that config mode has been disabled)");
		startDataUpdateCounter();

		onceTicker.once_ms(5000, configModePersistenceTestEnd);
	}
	else {
		testEnd(false, "Config mode has been disabled according to isConfigModeEnabled().");
	}
}


void configModePersistenceTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for requestFirmwareAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		configModePersistenceTestCheckConfigMode();
	}
	else {
		testEnd(false, "Test has failed due to requestFirmwareAsync() reporting ", asyncCommandResultToString(asyncResult));
	}
}

void configModePersistenceTest() {
	testStart("Config Mode Persistenc Test", "Checks whther config mode remains active when other commands are sent");

	bool ret = ld2410.requestAllStaticDataAsync(configModePersistenceTestCallback);
	if (ret) {
		testPrint("requestFirmwareAsync() started. Waiting for callback (which should not change the config mode state).");
	}
	else {
		testEnd(false, "requestFirmwareAsync() has returned false. This should only happen if another async command is pending pending");
	}

}

/********************************************************************************************************************************
** Disable config mode test
********************************************************************************************************************************/
void disableConfigModeTestEnd() {
	stopDataUpdateCounter();
	if (dataUpdateCounter_engineeringModeCount != 0 || dataUpdateCounter_normalModeCount != 0) {
		Serial.println();
		testEnd(true, "Config mode has been disabled successfully");
	}
	else {
		testEnd(false, "disableConfigModeAsync() reports success, but the LD2410 does not send detection data (that typically means that config mode is active).");
	}
}

void disableConfigModeTestCheckForDataUpdates() {
	testPrint("Waiting a 5 seconds, so we can be sure that config mode has really been disabled (checking if data updates are sent).");

	startDataUpdateCounter();
	onceTicker.once_ms(5000, disableConfigModeTestEnd);
}



void  disableConfigModeTestDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for disableConfigModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		disableConfigModeTestCheckForDataUpdates();
	}
	else {
		testEnd(false, "Test has failed due to ", asyncCommandResultToString(asyncResult));
	}
}


void disableConfigModeTest() {
	testStart("disableConfigModeAsync() Test", "Disables config mode. LD2410 will return to normal data detection.");

	bool ret = ld2410.disableConfigModeAsync(disableConfigModeTestDisableConfigModeCallback);

	if (ret) {
		testPrint("disableConfigModeAsync() conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "disableConfigModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}


/********************************************************************************************************************************
** Disable disabled config mode test
********************************************************************************************************************************/

void disableDisabledConfigModeTestDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for disableConfigModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true);
	}
	else {
		testEnd(false, "Test has failed due to ", asyncCommandResultToString(asyncResult));
	}
}

void disableDisabledConfigModeTest() {
	testStart("disableConfigModeAsync() Test when config mode is already disabled", "Disables config mode, but config mode is not active. The command detects that config mode is inactive and fires the callback avter a minor (1ms) delay.");

	bool ret = ld2410.disableConfigModeAsync(disableDisabledConfigModeTestDisableConfigModeCallback);

	if (ret) {
		testPrint("disableConfigModeAsync() conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "disableConfigModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}

/********************************************************************************************************************************
** Disable disabled config mode test
********************************************************************************************************************************/

void forceDisableDisabledConfigModeTestDisableDisabledConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for disableConfigModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));

	if (asyncResult == LD2410Async::AsyncCommandResult::TIMEOUT) {
		testEnd(true, "Command has timmed out as expected");
	}
	else {
		testEnd(false, "Test has failed due to command returning ", asyncCommandResultToString(asyncResult));
	}
}
void forceDisableDisabledConfigModeTest() {
	testStart("Forcing disableConfigModeAsync() Test when config mode is already disabled", "Since config mode is already inactive, the sensor will not send an ACK on the command and therefore the command will timeout");

	bool ret = ld2410.disableConfigModeAsync(true, forceDisableDisabledConfigModeTestDisableDisabledConfigModeCallback);

	if (ret) {
		testPrint("disableConfigModeAsync() with force para conmpleted. Expecting callback.");
	}
	else {
		testEnd(false, "disableConfigModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}

/********************************************************************************************************************************
** asyncIsBusy() Test
********************************************************************************************************************************/

int asyncIsBusyTest_Count = 0;

void asyncIsBusyTestDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true, "asyncIsBusy() reported true / busy ", asyncIsBusyTest_Count, " times.");
	}
	else {
		testEnd(false, "Test has failed due to diableConfigModeAsync() returning ", asyncCommandResultToString(asyncResult), ". Cant complete asyncIsBusy() test");
	}
}

void asyncIsBusyTestEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	onceTicker.detach();

	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testPrint("asyncIsBusy() reported true/busy ", asyncIsBusyTest_Count, " times while waiting for the enableConfigModeAsync() command to complete.");

		bool ret = ld2410.disableConfigModeAsync(asyncIsBusyTestDisableConfigModeCallback);

		if (ret) {
			testPrint("Waiting for disableConfigModeAsync() to complete test.");
		}
		else {
			testEnd(false, "disableConfigModeAsync() has returned false. Cant complete asyncIsBusy() test");
		}
	}
	else {
		testEnd(false, "Test has failed due to enableConfigModeAsync() returning ", asyncCommandResultToString(asyncResult), ". Cant complete asyncIsBusy() test");
	}
}

void asyncIsBusyTestCheckBusy() {
	if (ld2410.asyncIsBusy()) {
		asyncIsBusyTest_Count++;
	}

	onceTicker.once_ms(20, asyncIsBusyTestCheckBusy);
}


void asyncIsBusyTest() {
	testStart("asyncIsBusyTest() Test", "Tries to enable config mode and checks for busy while waiting for the ack");

	bool ret = ld2410.enableConfigModeAsync(asyncIsBusyTestEnableConfigModeCallback);

	if (ret) {
		testPrint("enableConfigModeAsync() started. Checking for asyncIsBusy() while waiting for callback.");
		asyncIsBusyTest_Count = 0;
		asyncIsBusyTestCheckBusy();
	}
	else {
		testEnd(false, "enableConfigModeAsync() has returned false. Cant execute asyncIsBusy() test");
	}

}


/********************************************************************************************************************************
** asyncCancel() Test
********************************************************************************************************************************/

int asyncCancelTest_Count = 0;

void asyncCancelTestDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true, "asyncCancel() has successfully canceled the disableConfigModeAsync() command");
	}
	else {
		testEnd(false, "Test has failed due to disableConfigModeAsync() returning ", asyncCommandResultToString(asyncResult), ". Cant complete asyncCancel() test");
	}
}

void asyncCancelTestDiableConfigMode() {
	bool ret = ld2410.disableConfigModeAsync(asyncCancelTestDisableConfigModeCallback);

	if (ret) {
		testPrint("Waiting for disableConfigModeAsync() callback to complete test.");
	}
	else {
		testEnd(false, "disableConfigModeAsync() has returned false. Cant complete asyncCancel() test");
	}
}

void asyncCancelTestEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	onceTicker.detach();

	if (asyncResult == LD2410Async::AsyncCommandResult::CANCELED) {
		testPrint("Calledback reportet CANCELED as expected.");
		testPrint("Will wait 5 secs so the disableConfigModeAsync() used for the test can complete, before we disable config mode again.");

		onceTicker.once_ms(5000, asyncCancelTestDiableConfigMode);


	}
	else {
		testEnd(false, "Test has failed due to enableConfigModeAsync() returning ", asyncCommandResultToString(asyncResult), ". Cant complete asyncCancel() test");
	}
}

void asyncCancelCancelCommand() {
	testPrint("Executing asyncCancel()");
	ld2410.asyncCancel();
}


void asyncCancelTest() {
	testStart("asyncCancelTest() Test", "Tries to enable config mode and cancels command.");

	bool ret = ld2410.enableConfigModeAsync(asyncCancelTestEnableConfigModeCallback);

	if (ret) {
		testPrint("enableConfigModeAsync() started. Checking for asyncCancel() while waiting for callback.");
		onceTicker.once_ms(20, asyncCancelCancelCommand);
	}
	else {
		testEnd(false, "enableConfigModeAsync() has returned false. Cant execute asyncCancel() test");
	}

}


/********************************************************************************************************************************
** requestGateParametersAsync() Test
********************************************************************************************************************************/
bool hasGateParameterConfigDataBeenReceived(const LD2410Types::ConfigData& cfg) {
	bool valid = false;

	for (size_t i = 0; i < 9; i++)
	{
		if (cfg.distanceGateMotionSensitivity[i] > 0 || cfg.distanceGateStationarySensitivity[i] > 0) {
			valid = true;
			break;
		}
	}

	if (valid) {
		valid = cfg.maxMotionDistanceGate > 0
			&& cfg.maxMotionDistanceGate <= 8
			&& cfg.maxStationaryDistanceGate > 0
			&& cfg.maxStationaryDistanceGate <= 8
			&& cfg.numberOfGates > 0;
	}
	return valid;
}

void requestGateParametersAsyncTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		const LD2410Types::ConfigData& cfg = sender->getConfigDataRef();

		cfg.print();
		if (!hasGateParameterConfigDataBeenReceived(cfg))
		{
			testEnd(false, "requestGateParametersAsyncTest() callback reports success, but the received data is invalid or no data has been received");
		}
		else {
			testEnd(true, "requestGateParametersAsyncTest() callback reports success and valid data has been received");
		}
	}
	else {
		testEnd(false, "requestGateParametersAsyncTest has failed. Command result: ", asyncCommandResultToString(asyncResult));
	}
}

void requestGateParametersAsyncTest() {
	testStart("requestGateParametersAsync() Test");

	ld2410.resetConfigData();
	bool ret = ld2410.requestGateParametersAsync(requestGateParametersAsyncTestCallback);

	if (ret) {
		testPrint("requestGateParametersAsync() started. Waiting for callback.");

	}
	else {
		testEnd(false, "requestGateParametersAsync() has returned false. Cant execute test");
	}
}

/********************************************************************************************************************************
** requestFirmwareAsync() Test
********************************************************************************************************************************/

void requestFirmwareAsyncTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true, "requestFirmwareAsync() callback reports success");
	}
	else {
		testEnd(false, "requestFirmwareAsyncTest has failed. Command result: ", asyncCommandResultToString(asyncResult));
	}
}

void requestFirmwareAsyncTest() {
	testStart("requestFirmwareAsync() Test");

	bool ret = ld2410.requestFirmwareAsync(requestFirmwareAsyncTestCallback);

	if (ret) {
		testPrint("requestFirmwareAsync() started. Waiting for callback.");

	}
	else {
		testEnd(false, "requestFirmwareAsync() has returned false. Cant execute test");
	}
}

/********************************************************************************************************************************
** requestBluetoothMacAddressAsync() Test
********************************************************************************************************************************/

void requestBluetoothMacAddressAsyncTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testEnd(true, "requestBluetoothMacAddressAsync() callback reports success");
	}
	else {
		testEnd(false, "requestBluetoothMacAddressAsyncTest has failed. Command result: ", asyncCommandResultToString(asyncResult));
	}
}

void requestBluetoothMacAddressAsyncTest() {
	testStart("requestBluetoothMacAddressAsync() Test");

	bool ret = ld2410.requestBluetoothMacAddressAsync(requestBluetoothMacAddressAsyncTestCallback);

	if (ret) {
		testPrint("requestBluetoothMacAddressAsync() started. Waiting for callback.");

	}
	else {
		testEnd(false, "requestBluetoothMacAddressAsync() has returned false. Cant execute test");
	}
}

/********************************************************************************************************************************
** requestDistanceResolutionCmAsync() Test
********************************************************************************************************************************/

void requestDistanceResolutionCmAsyncTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		const LD2410Types::ConfigData& cfg = sender->getConfigDataRef();
		if (cfg.distanceResolution == LD2410Types::DistanceResolution::NOT_SET)
		{
			testEnd(false, "requestDistanceResolutionCmAsync() callback reports success, but the received data is invalid or no data has been received");
		}
		else {
			testEnd(true, "requestDistanceResolutionCmAsync() callback reports success and valid data has been received");
		}
	}
	else {
		testEnd(false, "requestDistanceResolutionCmAsyncTest has failed. Command result: ", asyncCommandResultToString(asyncResult));
	}
}

void requestDistanceResolutionCmAsyncTest() {
	testStart("requestDistanceResolutionCmAsync() Test");
	ld2410.resetConfigData();
	bool ret = ld2410.requestDistanceResolutionCmAsync(requestDistanceResolutionCmAsyncTestCallback);

	if (ret) {
		testPrint("requestDistanceResolutionCmAsync() started. Waiting for callback.");

	}
	else {
		testEnd(false, "requestDistanceResolutionCmAsync() has returned false. Cant execute test");
	}
}


/********************************************************************************************************************************
** requestAuxControlSettingsAsync() Test
********************************************************************************************************************************/

void requestAuxControlSettingsAsyncTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		const LD2410Types::ConfigData& cfg = sender->getConfigDataRef();
		if (cfg.lightControl == LD2410Types::LightControl::NOT_SET
			|| cfg.outputControl == LD2410Types::OutputControl::NOT_SET
			)
		{
			testEnd(false, "requestAuxControlSettingsAsync() callback reports success, but the received data is invalid or no data has been received");
		}
		else {
			testEnd(true, "requestAuxControlSettingsAsync() callback reports success and valid data has been received");
		}
	}
	else {
		testEnd(false, "requestAuxControlSettingsAsyncTest has failed. Command result: ", asyncCommandResultToString(asyncResult));
	}
}

void requestAuxControlSettingsAsyncTest() {
	testStart("requestAuxControlSettingsAsync() Test");

	ld2410.resetConfigData();

	bool ret = ld2410.requestAuxControlSettingsAsync(requestAuxControlSettingsAsyncTestCallback);

	if (ret) {
		testPrint("requestAuxControlSettingsAsync() started. Waiting for callback.");

	}
	else {
		testEnd(false, "requestAuxControlSettingsAsync() has returned false. Cant execute test");
	}
}

/********************************************************************************************************************************
** Inactivity handling Test
********************************************************************************************************************************/

void inactivityHandlingTestEnd() {
	stopDataUpdateCounter();

	ld2410.setInactivityTimeoutMs(60000); //Set inactivity timeout back to 60 secs 

	if (dataUpdateCounter_engineeringModeCount == 0 && dataUpdateCounter_normalModeCount == 0) {
		testEnd(false, "Inactivity test has failed. LD2410 did not go back to normal mode inactivity handling.");
	}
	else {

		testEnd(true, "Inactivity test has passed. LD2410 has started to send data after inactivity handling.");
	}
}

void inactivityHandlingTestEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for enableConfigModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testPrint("LD2410 is now in config mode. Waiting 25 secs to see whether it goes back to detection mode after inactivity timeout (will try cancel without effect first and then try exit config mode).");

		startDataUpdateCounter();

		onceTicker.once_ms(25000, inactivityHandlingTestEnd); //Wait 25 secs, so we are sure that the inactivity timeout of 10 secs has passed and 2 attempts to recover (cancel async and exit config mode) have been made
	}
	else {
		testEnd(false, "Test has failed due enableConfigModeAsync() failure. Return value: ", asyncCommandResultToString(asyncResult));
	}
}

void inactivityHandlingTest() {
	testStart("Inactivity Handling Test");

	ld2410.enableInactivityHandling();

	ld2410.setInactivityTimeoutMs(10000); //10 secs

	if (ld2410.getInactivityTimeoutMs() != 10000) {
		testEnd(false, "Inactivity timeout could not be set correctly");
		return;
	}

	testPrint("Inacctivity handling enabled and timeout set to 10 secs");
	testPrint("Will activate config mode, remain sillent and wait to see if sensor goes back to datection mode automatically.");

	bool ret = ld2410.enableConfigModeAsync(inactivityHandlingTestEnableConfigModeCallback);
	if (ret) {
		testPrint("enableConfigModeAsync() executed. Expecting callback.");
	}
	else {
		testEnd(false, "enableConfigModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}

/********************************************************************************************************************************
** Inactivity Handling Disable Test
********************************************************************************************************************************/
void disableInactivityHandlingTestEnd() {
	stopDataUpdateCounter();
	ld2410.setInactivityTimeoutMs(60000); //Set inactivity timeout back to 60 secs 
	ld2410.enableInactivityHandling();	//reenable inactivity handling

	if (dataUpdateCounter_engineeringModeCount == 0 && dataUpdateCounter_normalModeCount == 0) {
		testEnd(true, "Disable inactivity handling test has passed. LD2410 has not gone back into detection mode.");
	}
	else {
		testEnd(false, "Disable inactivity handling test has failed. LD2410 did go back into detection mode.");
	}
}


void disableInactivityHandlingTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	testPrint("Callback for enableConfigModeAsync() executed. Result: ", asyncCommandResultToString(asyncResult));
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		testPrint("Will wait for 15 secs to see whether the LD2410 goes back to detection mode (it should not do this since inactivity handling is disabled).");

		startDataUpdateCounter();

		onceTicker.once_ms(15000, disableInactivityHandlingTestEnd); //Wait 15 secs, so we are sure that the inactivity timeout of 10 secs has passed


	}
	else {
		testEnd(false, "Test has failed due enableConfigModeAsync() failure. Return value: ", asyncCommandResultToString(asyncResult));
	}
}


void disableInactivityHandlingTest() {
	testStart("Disable Inactivity Handling Test");

	testPrint("DIsabling inactivity handling and setting a 10 secs timeout");
	ld2410.setInactivityTimeoutMs(10000); //10 secs
	ld2410.disableInactivityHandling();
	if (ld2410.isInactivityHandlingEnabled()) {
		testEnd(false, "Inactivity handling could not be disabled");
		return;
	}
	bool ret = ld2410.enableConfigModeAsync(disableInactivityHandlingTestCallback);
	if (ret) {
		testPrint("enableConfigModeAsync() executed. Expecting callback.");
	}
	else {
		testEnd(false, "enableConfigModeAsync() has returned false. This should only happen if another async command is pending pending");
	}
}


/********************************************************************************************************************************
** Config data struct validation test
********************************************************************************************************************************/

void configDataStructValidationTest() {
	testStart("ConfigData Struct Validation Test");

	testPrint("Initialize new, empty config data struct.");
	LD2410Types::ConfigData testConfigData;

	if (testConfigData.isValid()) {
		testEnd(false, "Newly initialized config data struct is valid. This should not be the case.");
		return;
	}
	else {
		testPrint("Newly initialized config data struct is not valid as expected.");
	}

	testPrint("Initialize config data struct with valid values.");
	initializeConfigDataStruct(testConfigData);

	if (!testConfigData.isValid()) {
		testEnd(false, "Config data struct is not valid (but has been initialized with values.");
		return;
	}
	else {
		testPrint("Config data struct is valid");
	}


	testConfigData.lightControl = LD2410Types::LightControl::NOT_SET;
	if (testConfigData.isValid()) {
		testEnd(false, "Config data struct is valid, but lightControl value is NOT_SET (which is invalid)");
		return;
	}
	else {
		testPrint("Config data struct with lightControl value NOT_SET is not valid as expected.");
	}
	testConfigData.lightControl = LD2410Types::LightControl::LIGHT_ABOVE_THRESHOLD;

	testConfigData.outputControl = LD2410Types::OutputControl::NOT_SET;
	if (testConfigData.isValid()) {
		testEnd(false, "Config data struct is valid, but outputControl value is NOT_SET (which is invalid)");
		return;
	}
	else {
		testPrint("Config data struct with outputControl value NOT_SET is not valid as expected.");
	}
	testConfigData.outputControl = LD2410Types::OutputControl::DEFAULT_LOW_DETECTED_HIGH;

	testConfigData.distanceResolution = LD2410Types::DistanceResolution::NOT_SET;
	if (testConfigData.isValid()) {
		testEnd(false, "Config data struct is valid, but distanceResolution value is NOT_SET (which is invalid)");
		return;
	}
	else {
		testPrint("Config data struct with distanceResolution value NOT_SET is not valid as expected.");
	};
	testConfigData.distanceResolution = LD2410Types::DistanceResolution::RESOLUTION_75CM;

	for (int i = 0; i < 9; i++) {
		testConfigData.distanceGateMotionSensitivity[i] = 210;
		if (testConfigData.isValid()) {
			testEnd(false, "distanceGateMotionSensitivity is out of range, but config data struct is reported as valid");
			return;
		}
		testConfigData.distanceGateMotionSensitivity[i] = 5 + 5 * i;

		testConfigData.distanceGateStationarySensitivity[i] = 200;
		if (testConfigData.isValid()) {
			testEnd(false, "distanceGateStationarySensitivity is out of range, but config data struct is reported as valid");
			return;
		}

		testConfigData.distanceGateStationarySensitivity[i] = 100 - i;
	}
	testPrint("Checking distanceGateMotionSensitivity and distanceGateStationarySensitivity with out of range values reported invalid as excpected.");


	testConfigData.maxMotionDistanceGate = 0;
	if (testConfigData.isValid()) {
		testEnd(false, "maxMotionDistanceGate is out of range, but config data struct is reported as valid");
		return;
	};
	testConfigData.maxMotionDistanceGate = 5;
	testConfigData.maxStationaryDistanceGate = 10;
	if (testConfigData.isValid()) {
		testEnd(false, "maxStationaryDistanceGate is out of range, but config data struct is reported as valid");
		return;
	};
	testConfigData.maxStationaryDistanceGate = 4;

	testEnd(true, "Config data struct validation test has passed.");
}


/********************************************************************************************************************************
** requestAllConfigSettingsAsync() Test
********************************************************************************************************************************/

void requestAllConfigSettingsAsyncTestCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult asyncResult, byte userData) {
	if (asyncResult == LD2410Async::AsyncCommandResult::SUCCESS) {
		const LD2410Types::ConfigData& cfg = sender->getConfigDataRef();
		if (cfg.lightControl == LD2410Types::LightControl::NOT_SET
			|| cfg.outputControl == LD2410Types::OutputControl::NOT_SET
			|| !hasGateParameterConfigDataBeenReceived(cfg))
		{
			testEnd(false, "requestAllConfigSettingsAsync() callback reports success, but no data has been received or data is invalid.");
		}
		else {
			testEnd(true, "requestAllConfigSettingsAsync() callback reports success and data has been received");
		}
	}
	else {
		testEnd(false, "requestAllConfigSettingsAsyncTest has failed. Command result: ", asyncCommandResultToString(asyncResult));
	}
}

void requestAllConfigSettingsAsyncTest() {
	testStart("requestAllConfigSettingsAsync() Test");

	ld2410.resetConfigData();
	bool ret = ld2410.requestAllConfigSettingsAsync(requestAllConfigSettingsAsyncTestCallback);

	if (ret) {
		testPrint("requestAllConfigSettingsAsync() started. Waiting for callback.");

	}
	else {
		testEnd(false, "requestAllConfigSettingsAsync() has returned false. Cant execute test");
	}
}

/********************************************************************************************************************************
** getConfigData() & getConfigDataRef() Test
********************************************************************************************************************************/



void getConfigDataTest() {
	testStart("getConfigData() & getConfigDataRef() Test");

	testPrint("Getting a clone of the config data struct with getConfigData()");
	orgConfigData = ld2410.getConfigData();
	testPrint("The cloned struct has been stored in variable orgConfigData so we can use it to restore the org data at the end of the tests");

	testPrint("Getting a reference to the config data struct with getConfigDataRef()");
	const LD2410Types::ConfigData& configDataRef = ld2410.getConfigDataRef();

	testPrint("Check if the are both the same (they should be)");
	if (!orgConfigData.equals(configDataRef)) {
		testEnd(false, "getConfigData() test has failed. The two config data structs are not equal.");
		return;
	}
	testPrint("The cloned config data struct and the referenced struct are the same as expected.");
	testEnd(true, "getConfigData() & getConfigDataRef() test has passed.");

}




/********************************************************************************************************************************
** Test sequence
** Defines the sequence of the tests
********************************************************************************************************************************/





typedef void (*TestAction)();


TestAction actions[] = {
	beginTest,
	rebootTest,
	normalModeDataReceiveTest,
	enableEngineeringModeTest,
	engineeringModeDataReceiveTest,
	disableEngineeringModeTest,
	enableConfigModeTest,
	configModePersistenceTest,
	disableConfigModeTest,
	disableDisabledConfigModeTest,
	forceDisableDisabledConfigModeTest,
	asyncIsBusyTest,
	asyncCancelTest,
	inactivityHandlingTest,
	disableInactivityHandlingTest,

	//Config data Tests
	configDataStructValidationTest,
	requestAuxControlSettingsAsyncTest,
	requestDistanceResolutionCmAsyncTest,
	requestGateParametersAsyncTest,

	requestAllConfigSettingsAsyncTest, //Fetches all the current config data. A clone of that data will be used for the next few tests
	getConfigDataTest,	//This test stores the org config data which was requested during the requestAllConfigSettingsAsyncTest in orgConfigData, so we can set the org config back after the tests

	//The config tests below are all cloning the orgConfigData that was requested and set in the previous two steps.
	//The next few tests update config data on the sensor. This causes wear on the non valodatile memory of the sensor. Do do those tests to often





	//Static data tests
	requestBluetoothMacAddressAsyncTest,
	requestFirmwareAsyncTest,

};

const int NUM_ACTIONS = sizeof(actions) / sizeof(actions[0]);


/********************************************************************************************************************************
** Start next test
** Executes the next test in the test sequence
********************************************************************************************************************************/

void startNextTest() {
	currentTestIndex++;
	if (currentTestIndex < NUM_ACTIONS) {
		actions[currentTestIndex]();
	}
	else {
		// all tests completed successfully
		printBigMessage("ALL TESTS PASSED", '-');
		testSequenceRunning = false;
	}
}
/********************************************************************************************************************************
** Starts the first test
********************************************************************************************************************************/
void startTests() {
	currentTestIndex = -1;
	testSequenceRunning = true;
	startNextTest();
};

/********************************************************************************************************************************
** Setup
********************************************************************************************************************************/
void setup() {
	//Using a big output buffer, since this test sketch prints a lot of data.
	Serial.setTxBufferSize(2048);
	// Initialize USB serial for debug output
	Serial.begin(115200);
	while (!Serial) {
		; // wait for Serial Monitor
	}

	// Initialize Serial1 with user-defined pins and baudrate
	RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

	//Delay a few ms to ensure that Serial is available
	delay(10);

	//Start the tests
	startTests();

}

/********************************************************************************************************************************
** Loop
** Loop is empty since all test are executed in a async fashion
********************************************************************************************************************************/
void loop() {
	//Nothing to do
	delay(1000);
}
