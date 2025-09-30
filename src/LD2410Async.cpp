#include "Arduino.h"
#include "Ticker.h"
#include "LD2410Async.h"
#include "LD2410Defs.h"
#include "LD2410Types.h"
#include "LD2410CommandBuilder.h"



/******************************************************************************************
* Utility methods
******************************************************************************************/

/**
* @brief Checks whther the last 4 bytes of a buffer match the value supplied in pattern
*/
bool bufferEndsWith(const byte* buffer, int iMax, const byte* pattern)
{
	for (int j = 3; j >= 0; j--)
	{
		--iMax;
		if (buffer[iMax] != pattern[j]) {
			return false;
		}
	}
	return true;
}

String byte2hex(byte b, bool addZero = true)
{
	String bStr(b, HEX);
	bStr.toUpperCase();
	if (addZero && (bStr.length() == 1))
		return "0" + bStr;
	return bStr;
}




/******************************************************************************************
* Read frame methods
******************************************************************************************/
bool LD2410Async::readFramePayloadSize(byte b, ReadFrameState nextReadFrameState) {
	receiveBuffer[receiveBufferIndex++] = b;
	if (receiveBufferIndex >= 2) {
		payloadSize = receiveBuffer[0] | (receiveBuffer[1] << 8);
		if (payloadSize <= 0 || payloadSize > sizeof(receiveBuffer) - 4) {
			//Serial.print("Invalid payload size read: ");
			//Serial.print(payloadSize);
			//Serial.print(" ");
			//printBuf(receiveBuffer, 2);

			//Invalid payload size, wait for header.
			readFrameState = ReadFrameState::WAITING_FOR_HEADER;
		}
		else {

			//Serial.print("Payloadsize read: ");
			//Serial.print(payloadSize);
			//Serial.print(" ");
			//printBuf(receiveBuffer, 2);


			receiveBufferIndex = 0;
			readFrameState = nextReadFrameState;
		}
		return true;
	}
	return false;
}

LD2410Async::FrameReadResponse LD2410Async::readFramePayload(byte b, const byte* tailPattern, LD2410Async::FrameReadResponse succesResponseType) {
	receiveBuffer[receiveBufferIndex++] = b;

	//Add 4 for the tail bytes
	if (receiveBufferIndex >= payloadSize + 4) {

		readFrameState = ReadFrameState::WAITING_FOR_HEADER;

		if (bufferEndsWith(receiveBuffer, receiveBufferIndex, tailPattern)) {
			//Serial.println("Payload read: ");
			//printBuf(receiveBuffer, receiveBufferIndex);

			return succesResponseType;
		}
		else {
			//Serial.print(" Invalid frame end: ");
			//printBuf(receiveBuffer, receiveBufferIndex);

		}

	}
	return FAIL;
};

LD2410Async::FrameReadResponse LD2410Async::readFrame()
{
	while (sensor->available()) {
		byte b = sensor->read();

		switch (readFrameState) {
		case WAITING_FOR_HEADER:
			if (b == LD2410Defs::headData[0]) {
				readFrameHeaderIndex = 1;
				readFrameState = DATA_HEADER;

			}
			else if (b == LD2410Defs::headConfig[0]) {
				readFrameHeaderIndex = 1;
				readFrameState = ACK_HEADER;

			}
			break;

		case DATA_HEADER:
			if (b == LD2410Defs::headData[readFrameHeaderIndex]) {
				readFrameHeaderIndex++;

				if (readFrameHeaderIndex == sizeof(LD2410Defs::headData)) {
					receiveBufferIndex = 0;
					readFrameState = READ_DATA_SIZE;
				}
			}
			else if (b == LD2410Defs::headData[0]) {
				// fallback: this byte might be the start of a new header
				readFrameHeaderIndex = 1;

				// stay in DATA_HEADER
			}
			else if (b == LD2410Defs::headConfig[0]) {
				// possible start of config header
				readFrameHeaderIndex = 1;
				readFrameState = ACK_HEADER;

			}
			else {
				// not a header at all
				readFrameState = WAITING_FOR_HEADER;
				readFrameHeaderIndex = 0;

			}
			break;

		case ACK_HEADER:
			if (b == LD2410Defs::headConfig[readFrameHeaderIndex]) {
				readFrameHeaderIndex++;

				if (readFrameHeaderIndex == sizeof(LD2410Defs::headConfig)) {
					receiveBufferIndex = 0;
					readFrameState = READ_ACK_SIZE;
				}
			}
			else if (b == LD2410Defs::headConfig[0]) {
				readFrameHeaderIndex = 1;
				// stay in ACK_HEADER

			}
			else if (b == LD2410Defs::headData[0]) {
				// maybe start of data header
				readFrameHeaderIndex = 1;
				readFrameState = DATA_HEADER;

			}
			else {
				readFrameState = WAITING_FOR_HEADER;
				readFrameHeaderIndex = 0;


			}
			break;

		case READ_ACK_SIZE:
			readFramePayloadSize(b, READ_ACK_PAYLOAD);
			break;

		case READ_DATA_SIZE:
			readFramePayloadSize(b, READ_DATA_PAYLOAD);
			break;

		case READ_ACK_PAYLOAD:
			return readFramePayload(b, LD2410Defs::tailConfig, ACK);

		case READ_DATA_PAYLOAD:
			return readFramePayload(b, LD2410Defs::tailData, DATA);

		default:
			readFrameState = WAITING_FOR_HEADER;
			readFrameHeaderIndex = 0;
			break;
		}
	}

	return FAIL; // not enough bytes yet
}


/******************************************************************************************
* Generic Callbacks
******************************************************************************************/
void LD2410Async::registerDetectionDataReceivedCallback(DetectionDataCallback callback, byte userData) {
	detectionDataCallback = callback;
	detectionDataCallbackUserData = userData;
}

void LD2410Async::registerConfigUpdateReceivedCallback(GenericCallback callback, byte userData) {

	configUpdateReceivedReceivedCallbackUserData = userData;
	configUpdateReceivedReceivedCallback = callback;
}

void LD2410Async::registerConfigChangedCallback(GenericCallback callback, byte userData) {
	configChangedCallbackUserData = userData;
	configChangedCallback = callback;
}



void LD2410Async::executeConfigUpdateReceivedCallback() {

	if (configUpdateReceivedReceivedCallback) {
		configUpdateReceivedReceivedCallback(this, configUpdateReceivedReceivedCallbackUserData);
	}

}

void LD2410Async::executeConfigChangedCallback() {

	if (configChangedCallback) {
		configChangedCallback(this, configChangedCallbackUserData);
	}
}
/******************************************************************************************
* Process received data
******************************************************************************************/
bool LD2410Async::processAck()
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTBUF(receiveBuffer, receiveBufferIndex)

		heartbeat();

	byte command = receiveBuffer[0];
	byte success = receiveBuffer[1];

	if (!success) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("FAIL for command: ");
		DEBUG_PRINTLN(byte2hex(command));
		executeAsyncCommandCallback(command, LD2410Async::AsyncCommandResult::FAILED);
		return false;
	};


	switch (command)
	{
	case LD2410Defs::configEnableCommand: // entered config mode
		configModeEnabled = true;
		staticData.protocolVersion = receiveBuffer[4] | (receiveBuffer[5] << 8);
		staticData.bufferSize = receiveBuffer[6] | (receiveBuffer[7] << 8);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for config mode enable received");
		break;
	case LD2410Defs::configDisableCommand: // exited config mode
		configModeEnabled = false;
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for config mode disable received");
		break;
	case LD2410Defs::maxGateCommand:
		DEBUG_PRINTLN("ACK for maxGateCommand received");
		executeConfigChangedCallback();
		break;
	case LD2410Defs::engineeringModeEnableComand:
		engineeringModeEnabled = true;
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for engineeringModeEnableComand received");
		break;
	case LD2410Defs::engineeringModeDisableComand:
		engineeringModeEnabled = false;
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for engineeringModeDisableComand received");
		break;
	case LD2410Defs::setBaudRateCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for setBaudRateCommand received");
		break;
	case LD2410Defs::restoreFactorySettingsAsyncCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for restoreFactorySettingsAsyncCommand received");
		executeConfigChangedCallback();
		break;
	case LD2410Defs::rebootCommand:
		engineeringModeEnabled = false;
		configModeEnabled = false;
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for rebootCommand received");
		break;
	case LD2410Defs::bluetoothSettingsCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for bluetoothSettingsCommand received");
		break;
	case LD2410Defs::getBluetoothPermissionsCommand:
		//This command is only relevant for bluetooth connection, but the sensor might send an ack for it at some point.
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for getBluetoothPermissionsCommand received");
		break;
	case LD2410Defs::setBluetoothPasswordCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for setBluetoothPasswordCommand received");
		break;
	case LD2410Defs::setDistanceResolutionCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for setDistanceResolutionCommand received");
		executeConfigChangedCallback();
		break;
	case LD2410Defs::requestDistanceResolutionCommand:
		configData.distanceResolution = LD2410Types::toDistanceResolution(receiveBuffer[4]);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestDistanceResolutionCommand received");
		executeConfigUpdateReceivedCallback();
		break;
	case LD2410Defs::setAuxControlSettingsCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for setAuxControlSettingsCommand received");
		executeConfigChangedCallback();
		break;
	case LD2410Defs::requestMacAddressCommand:
		for (int i = 0; i < 6; i++) {
			staticData.bluetoothMac[i] = receiveBuffer[i + 4];
		}

		// Format MAC as "AA:BB:CC:DD:EE:FF"
		snprintf(staticData.bluetoothMacText, sizeof(staticData.bluetoothMacText),
			"%02X:%02X:%02X:%02X:%02X:%02X",
			staticData.bluetoothMac[0], staticData.bluetoothMac[1], staticData.bluetoothMac[2],
			staticData.bluetoothMac[3], staticData.bluetoothMac[4], staticData.bluetoothMac[5]);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestBluetoothMacAddressAsyncCommand received");
		break;
	case LD2410Defs::requestFirmwareCommand:
		snprintf(staticData.firmwareText, sizeof(staticData.firmwareText),
			"%X.%02X.%02X%02X%02X%02X",
			receiveBuffer[7],  // major (no leading zero)
			receiveBuffer[6],  // minor (keep 2 digits)
			receiveBuffer[11],
			receiveBuffer[10],
			receiveBuffer[9],
			receiveBuffer[8]);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestFirmwareAsyncCommand received");
		break;

	case LD2410Defs::requestAuxControlSettingsCommand:
		configData.lightControl = LD2410Types::toLightControl(receiveBuffer[4]);
		configData.lightThreshold = receiveBuffer[5];
		configData.outputControl = LD2410Types::toOutputControl(receiveBuffer[6]);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestAuxControlSettingsCommand received");
		executeConfigUpdateReceivedCallback();
		break;
	case LD2410Defs::beginAutoConfigCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for beginAutoConfigCommand received");
		break;
	case LD2410Defs::requestAutoConfigStatusCommand:
		autoConfigStatus = LD2410Types::toAutoConfigStatus(receiveBuffer[4]);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestAutoConfigStatusCommand received");
		break;
	case LD2410Defs::requestParamsCommand: // Query parameters
		configData.numberOfGates = receiveBuffer[5];
		configData.maxMotionDistanceGate = receiveBuffer[6];
		configData.maxStationaryDistanceGate = receiveBuffer[7];
		for (byte i = 0; i <= configData.numberOfGates; i++)			//Need to check if we should really do this or whther it just using a fixed number would be better
			configData.distanceGateMotionSensitivity[i] = receiveBuffer[8 + i];
		for (byte i = 0; i <= configData.numberOfGates; i++)
			configData.distanceGateStationarySensitivity[i] = receiveBuffer[17 + i];
		configData.noOneTimeout = receiveBuffer[26] | (receiveBuffer[27] << 8);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestGateParametersAsync received");
		executeConfigUpdateReceivedCallback();
		break;
	case LD2410Defs::distanceGateSensitivityConfigCommand:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for distanceGateSensitivityConfigCommand received");
		executeConfigChangedCallback();
		break;
	default:
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("ACK for unknown command received. Command code: ");
		DEBUG_PRINTLN(byte2hex(command));
		break;
	};

	if (command != 0) {
		executeAsyncCommandCallback(command, LD2410Async::AsyncCommandResult::SUCCESS);
	}


	return (true);
}

bool LD2410Async::processData()
{
	DEBUG_PRINTBUF_DATA(receiveBuffer, receiveBufferIndex)

		heartbeat();


	if (((receiveBuffer[0] == 1) || (receiveBuffer[0] == 2)) && (receiveBuffer[1] == 0xAA))
	{
		configModeEnabled = false;

		detectionData.timestamp = millis();
		//Basic data
		detectionData.targetState = LD2410Types::toTargetState(receiveBuffer[2] & 7);
		switch (detectionData.targetState) {
		case LD2410Types::TargetState::MOVING_TARGET:
			detectionData.presenceDetected = true;
			detectionData.movingPresenceDetected = true;
			detectionData.stationaryPresenceDetected = false;
			break;

		case LD2410Types::TargetState::STATIONARY_TARGET:
			detectionData.presenceDetected = true;
			detectionData.movingPresenceDetected = false;
			detectionData.stationaryPresenceDetected = true;
			break;

		case LD2410Types::TargetState::MOVING_AND_STATIONARY_TARGET:
			detectionData.presenceDetected = true;
			detectionData.movingPresenceDetected = true;
			detectionData.stationaryPresenceDetected = true;
			break;
		default:
			detectionData.presenceDetected = false;
			detectionData.movingPresenceDetected = false;
			detectionData.stationaryPresenceDetected = false;
			break;
		}
		detectionData.movingTargetDistance = receiveBuffer[3] | (receiveBuffer[4] << 8);
		detectionData.movingTargetSignal = receiveBuffer[5];
		detectionData.stationaryTargetDistance = receiveBuffer[6] | (receiveBuffer[7] << 8);
		detectionData.stationaryTargetSignal = receiveBuffer[8];
		detectionData.detectedDistance = receiveBuffer[9] | (receiveBuffer[10] << 8);
		detectionData.engineeringMode = receiveBuffer[0] == 1;

		engineeringModeEnabled = detectionData.engineeringMode;

		if (detectionData.engineeringMode)
		{ // Engineering mode data
			detectionData.movingTargetGateSignalCount = receiveBuffer[11];
			detectionData.stationaryTargetGateSignalCount = receiveBuffer[12];


			int index = 13;
			for (byte i = 0; i <= detectionData.movingTargetGateSignalCount; i++)
				detectionData.movingTargetGateSignals[i] = receiveBuffer[index++];
			for (byte i = 0; i <= detectionData.stationaryTargetGateSignalCount; i++)
				detectionData.stationaryTargetGateSignals[i] = receiveBuffer[index++];

			detectionData.outPinStatus = false;
			detectionData.lightLevel = 0;
			if (index < payloadSize) {
				detectionData.lightLevel = receiveBuffer[index++];
				if (index < payloadSize) {
					detectionData.outPinStatus = receiveBuffer[index++];
				}
			}
		}
		else
		{ // Clear engineering mode data
			detectionData.movingTargetGateSignalCount = 0;
			detectionData.stationaryTargetGateSignalCount = 0;
			detectionData.lightLevel = 0;
			detectionData.outPinStatus = false;
		}

		if (detectionDataCallback != nullptr) {
			detectionDataCallback(this, detectionData.presenceDetected, detectionDataCallbackUserData);
		};
		DEBUG_PRINTLN_DATA("DATA received");
		return true;
	}
	DEBUG_PRINTLN_DATA("DATA invalid");
	return false;
}






/******************************************************************************************
* Send command
******************************************************************************************/
void LD2410Async::sendCommand(const byte* command) {
	byte size = command[0] + 2;



	sensor->write(LD2410Defs::headConfig, 4);
	sensor->write(command, size);
	sensor->write(LD2410Defs::tailConfig, 4);

	heartbeat();
}

/**********************************************************************************
* Send async command methods
***********************************************************************************/
void LD2410Async::executeAsyncCommandCallback(byte commandCode, LD2410Async::AsyncCommandResult result) {
	if (asyncCommandActive && asyncCommandCommandCode == commandCode) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Async command duration ms: ");
		DEBUG_PRINTLN(millis() - asyncCommandStartMs);

		//Just to be sure that no other task changes the callback data or registers a callback before the callback has been executed
		vTaskSuspendAll();
		AsyncCommandCallback cb = asyncCommandCallback;
		byte userData = asyncCommandCallbackUserData;
		asyncCommandCallback = nullptr;
		asyncCommandCallbackUserData = 0;
		asyncCommandStartMs = 0;
		asyncCommandCommandCode = 0;
		asyncCommandActive = false;
		xTaskResumeAll();

		if (cb != nullptr) {
			cb(this, result, userData);
		}
	}
}




void LD2410Async::asyncCancel() {
	executeAsyncCommandCallback(asyncCommandCommandCode, LD2410Async::AsyncCommandResult::CANCELED);
}

void LD2410Async::handleAsyncCommandCallbackTimeout() {

	if (asyncCommandActive && asyncCommandStartMs != 0) {
		if (millis() - asyncCommandStartMs > asyncCommandTimeoutMs) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINT("Command timeout detected. Start time ms is: ");
			DEBUG_PRINT(asyncCommandStartMs);
			DEBUG_PRINTLN(". Execute callback with timeout result.");
			executeAsyncCommandCallback(asyncCommandCommandCode, LD2410Async::AsyncCommandResult::TIMEOUT);
		}
	}
}





bool LD2410Async::sendCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData)
{
	vTaskSuspendAll();

	//Dont check with asyncIsBusy() since this would also check if a sequence or setConfigDataASync is active
	if (!asyncCommandActive) {


		//Register data for callback
		asyncCommandActive = true;
		asyncCommandCallback = callback;
		asyncCommandCallbackUserData = userData;
		asyncCommandStartMs = millis();
		asyncCommandCommandCode = command[2];
		xTaskResumeAll();

		sendCommand(command);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Async command ");
		DEBUG_PRINT(byte2hex(command[2]));
		DEBUG_PRINTLN(" sent");

		return true;
	}
	else {
		xTaskResumeAll();
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Error! Async command is pending- Did not send async command ");
		DEBUG_PRINTLN(byte2hex(command[2]));

		return false;
	}


}
/**********************************************************************************
* Async command busy
***********************************************************************************/

bool LD2410Async::asyncIsBusy() {
	return asyncCommandActive || executeCommandSequenceActive || configureAllConfigSettingsAsyncConfigActive;
}

/**********************************************************************************
* Send async config command methods
***********************************************************************************/


bool LD2410Async::sendConfigCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData) {
	//Dont check with asyncIsBusy() since this would also check if a sequence or setConfigDataASync is active
	if (asyncCommandActive || executeCommandSequenceActive) return false;

	// Reset sequence buffer
	if (!resetCommandSequence()) return false;;

	// Add the single command
	if (!addCommandToSequence(command)) return false;

	// Execute as a sequence (with just one command)
	return executeCommandSequenceAsync(callback, userData);
}

/**********************************************************************************
* Async command sequence methods
***********************************************************************************/
void LD2410Async::executeCommandSequenceAsyncExecuteCallback(LD2410Async::AsyncCommandResult result) {
	if (executeCommandSequenceActive) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Command sequence duration ms: ");
		DEBUG_PRINTLN(millis() - executeCommandSequenceStartMs);

		vTaskSuspendAll();
		AsyncCommandCallback cb = executeCommandSequenceCallback;
		byte userData = executeCommandSequenceUserData;
		executeCommandSequenceCallback = nullptr;
		executeCommandSequenceUserData = 0;
		executeCommandSequenceActive = false;
		xTaskResumeAll();

		if (cb != nullptr) {
			cb(this, result, userData);
		}
	}
}


// Callback after disabling config mode at the end of sequence
void LD2410Async::executeCommandSequenceAsyncDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {

	LD2410Async::AsyncCommandResult sequenceResult = static_cast<LD2410Async::AsyncCommandResult>(userData);

	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Warning: Disabling config mode after command sequence failed. Result: ");
		DEBUG_PRINTLN(result);
		sequenceResult = result; // report disable failure if it happened
	}
	sender->executeCommandSequenceAsyncExecuteCallback(sequenceResult);
}

void LD2410Async::executeCommandSequenceAsyncFinalize(LD2410Async::AsyncCommandResult result) {
	if (!executeCommandSequenceInitialConfigModeState) {
		disableConfigModeAsync(executeCommandSequenceAsyncDisableConfigModeCallback, static_cast<byte>(result));
	}
	else {
		executeCommandSequenceAsyncExecuteCallback(result);
	}
}


// Called when a single command in the sequence completes
void LD2410Async::executeCommandSequenceAsyncCommandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		// Abort sequence if a command fails
		DEBUG_PRINT_MILLIS
			DEBUG_PRINT("Error: Command sequence aborted due to command failure. Result: ");
		sender->executeCommandSequenceAsyncFinalize(result);

		return;
	};

	// Move to next command
	sender->executeCommandSequenceIndex++;

	if (sender->executeCommandSequenceIndex < sender->commandSequenceBufferCount) {
		// Send next command
		sender->sendCommandAsync(
			sender->commandSequenceBuffer[sender->executeCommandSequenceIndex],
			executeCommandSequenceAsyncCommandCallback,
			0
		);
	}
	else {
		// Sequence finished successfully

		sender->executeCommandSequenceAsyncFinalize(LD2410Async::AsyncCommandResult::SUCCESS);
	}
}



bool LD2410Async::executeCommandSequenceAsync(AsyncCommandCallback callback, byte userData) {
	if (asyncCommandActive || executeCommandSequenceActive) return false;
	if (commandSequenceBufferCount == 0) return true; // nothing to send

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINT("Starting command sequence execution. Number of commands: ");
	DEBUG_PRINTLN(commandSequenceBufferCount);

	executeCommandSequenceActive = true;
	executeCommandSequenceCallback = callback;
	executeCommandSequenceUserData = userData;
	executeCommandSequenceInitialConfigModeState = configModeEnabled;
	executeCommandSequenceStartMs = millis();

	if (commandSequenceBufferCount == 0) {
		//Wait 1 ms to ensure that the callback is not executed before the caller of this method has finished its work
		executeCommandSequenceOnceTicker.once_ms(1, [this]() {
			this->executeCommandSequenceAsyncExecuteCallback(LD2410Async::AsyncCommandResult::SUCCESS);
			});
		return true;

	}

	if (!configModeEnabled) {
		// Need to enable config mode first
		// So set the sequence index to -1 to ensure the first command in the sequence (at index 0) is executed when the callback fires.
		executeCommandSequenceIndex = -1;
		return sendCommandAsync(LD2410Defs::configEnableCommandData, executeCommandSequenceAsyncCommandCallback, 0);
	}
	else {
		// Already in config mode, start directly. 
		// We start the first command in the sequence directly and set the sequence index 0, so the second command (if any) is executed when the callback fires. 
		executeCommandSequenceIndex = 0;
		return sendCommandAsync(commandSequenceBuffer[executeCommandSequenceIndex], executeCommandSequenceAsyncCommandCallback, 0);
	}

}

bool LD2410Async::addCommandToSequence(const byte* command) {
	if (asyncCommandActive || executeCommandSequenceActive) return false;

	if (commandSequenceBufferCount >= MAX_COMMAND_SEQUENCE_LENGTH) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Command sequence buffer full.");
		return false;
	};
	// First byte of the command is the payload length
	uint8_t len = command[0];
	uint8_t totalLen = len + 2; // payload + 2 length bytes

	if (totalLen > LD2410Defs::LD2410_Buffer_Size) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Command too long for command sequence buffer.");
		return false; // safety
	}
	memcpy(commandSequenceBuffer[commandSequenceBufferCount],
		command,
		totalLen);

	commandSequenceBufferCount++;
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINT("Command added to sequence. Count: ");
	DEBUG_PRINTLN(commandSequenceBufferCount);

	return true;
}


bool LD2410Async::resetCommandSequence() {
	if (asyncCommandActive || executeCommandSequenceActive) return false;

	commandSequenceBufferCount = 0;
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Command sequence reset done.");

	return true;
}

/**********************************************************************************
* Config mode commands
***********************************************************************************/
//Config mode command have a internal version which does not check for busy and can therefore be used within other command implementations

bool LD2410Async::enableConfigModeInternalAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable Config Mode");
	return sendCommandAsync(LD2410Defs::configEnableCommandData, callback, userData);
}

bool LD2410Async::enableConfigModeAsync(AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;
	return enableConfigModeInternalAsync(callback, userData);
}

bool LD2410Async::disableConfigModeInternalAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable Config Mode");
	return sendCommandAsync(LD2410Defs::configDisableCommandData, callback, userData);
}

bool LD2410Async::disableConfigModeAsync(AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;
	return disableConfigModeInternalAsync(callback, userData);
}

/**********************************************************************************
* Native LD2410 commands to control, configure and query the sensor
***********************************************************************************/
// All these commands need to be executed in config mode.
// The code takes care of that. Enables/disable config mode if necessary, 
// but also keeps config mode active if it was already active before the command
bool LD2410Async::configureMaxGateAndNoOneTimeoutAsync(byte maxMovingGate, byte maxStationaryGate,
	unsigned short noOneTimeout,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Max Gate");
	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::maxGateCommandData)];
	if (LD2410CommandBuilder::buildMaxGateCommand(cmd, maxMovingGate, maxStationaryGate, noOneTimeout)) {
		return sendConfigCommandAsync(cmd, callback, userData);
	}
	return false;
}


bool LD2410Async::requestGateParametersAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Gate Parameters");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::requestParamsCommandData, callback, userData);
}

bool LD2410Async::enableEngineeringModeAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable EngineeringMode");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::engineeringModeEnableCommandData, callback, userData);
}

bool LD2410Async::disableEngineeringModeAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable EngineeringMode");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::engineeringModeDisableCommandData, callback, userData);
}

bool LD2410Async::configureDistanceGateSensitivityAsync(const byte movingThresholds[9],
	const byte stationaryThresholds[9],
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Gate Sensitivities (all gates)");

	if (asyncIsBusy()) return false;

	if (!resetCommandSequence()) return false;

	for (byte gate = 0; gate < 9; gate++) {
		byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
		if (!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate, movingThresholds[gate], stationaryThresholds[gate])) return false;
		if (!addCommandToSequence(cmd)) return false;
	}

	return executeCommandSequenceAsync(callback, userData);
}



bool LD2410Async::configureDistanceGateSensitivityAsync(byte gate, byte movingThreshold,
	byte stationaryThreshold,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Gate Sensitivity");

	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
	if (!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate, movingThreshold, stationaryThreshold)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::requestFirmwareAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Firmware");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::requestFirmwareCommandData, callback, userData);
}


bool LD2410Async::configureBaudRateAsync(byte baudRateSetting,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Baud Rate");

	if (asyncIsBusy()) return false;

	if ((baudRateSetting < 1) || (baudRateSetting > 8))
		return false;

	byte cmd[sizeof(LD2410Defs::setBaudRateCommandData)];
	if (!LD2410CommandBuilder::buildBaudRateCommand(cmd, baudRateSetting)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::configureBaudRateAsync(LD2410Types::Baudrate baudRate, AsyncCommandCallback callback, byte userData) {

	if (asyncIsBusy()) return false;

	return configureBaudRateAsync((byte)baudRate, callback, userData);
}


bool LD2410Async::restoreFactorySettingsAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Restore Factory Settings");

	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::restoreFactorSettingsCommandData, callback, userData);
}



bool LD2410Async::enableBluetoothAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable Bluetooth");
	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::bluetoothSettingsOnCommandData, callback, userData);
}

bool LD2410Async::disableBluetoothAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable Bluetooth");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::bluetoothSettingsOnCommandData, callback, userData);
}


bool LD2410Async::requestBluetoothMacAddressAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Mac Address");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::requestMacAddressCommandData, callback, userData);
}

bool LD2410Async::configureBluetoothPasswordAsync(const char* password,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Bluetooth Password");
	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::setBluetoothPasswordCommandData)];
	if (!LD2410CommandBuilder::buildBluetoothPasswordCommand(cmd, password)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::configureBluetoothPasswordAsync(const String& password, AsyncCommandCallback callback, byte userData) {

	return configureBluetoothPasswordAsync(password.c_str(), callback, userData);
}


bool LD2410Async::configureDefaultBluetoothPasswordAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reset Bluetooth Password");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::setBluetoothPasswordCommandData, callback, userData);
}

bool LD2410Async::configureDistanceResolution75cmAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution 75cm");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::setDistanceResolution75cmCommandData, callback, userData);
};

bool LD2410Async::configureDistanceResolutionAsync(LD2410Types::DistanceResolution distanceResolution,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution");
	if (asyncIsBusy()) return false;

	byte cmd[6];
	if (!LD2410CommandBuilder::buildDistanceResolutionCommand(cmd, distanceResolution)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}

bool LD2410Async::configuresDistanceResolution20cmAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution 20cm");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::setDistanceResolution20cmCommandData, callback, userData);
};

bool LD2410Async::requestDistanceResolutioncmAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Distance Resolution cm");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::requestDistanceResolutionCommandData, callback, userData);
}

bool LD2410Async::configureAuxControlSettingsAsync(LD2410Types::LightControl lightControl, byte lightThreshold,
	LD2410Types::OutputControl outputControl,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Aux Control Settings");
	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::setAuxControlSettingCommandData)];
	if (!LD2410CommandBuilder::buildAuxControlCommand(cmd, lightControl, lightThreshold, outputControl)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::requestAuxControlSettingsAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Aux Control Settings");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::requestAuxControlSettingsCommandData, callback, userData);
}


bool LD2410Async::beginAutoConfigAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Begin auto config");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::beginAutoConfigCommandData, callback, userData);
};

bool LD2410Async::requestAutoConfigStatusAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reqtest auto config status");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::requestAutoConfigStatusCommandData, callback, userData);
}
/**********************************************************************************
* High level commands that combine several native commands
***********************************************************************************/
// It is recommend to always use these commands if feasible,
// since they streamline the inconsistencies in the native requesr and config commands
// and reduce the total number of commands that have to be called.


bool LD2410Async::requestAllStaticDataAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request all static data");

	if (asyncIsBusy()) return false;


	if (!resetCommandSequence()) return false;

	if (!addCommandToSequence(LD2410Defs::requestFirmwareCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestMacAddressCommandData)) return false;

	return executeCommandSequenceAsync(callback, userData);
}

bool LD2410Async::requestAllConfigSettingsAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request all config data");

	if (asyncIsBusy()) return false;

	if (!resetCommandSequence()) return false;
	if (!addCommandToSequence(LD2410Defs::requestDistanceResolutionCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestParamsCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestAuxControlSettingsCommandData)) return false;

	return executeCommandSequenceAsync(callback, userData);

}
// ----------------------------------------------------------------------------------
// - Set config data async methods
// ----------------------------------------------------------------------------------
// The command to set all config values on the sensor is the most complex command internally.
// It uses a first command sequences to get the current sensor config, then checks what 
// actually needs to be changed and then creates a second command sequence to do the needed changes.

void LD2410Async::configureAllConfigSettingsAsyncExecuteCallback(LD2410Async::AsyncCommandResult result) {
	AsyncCommandCallback cb = configureAllConfigSettingsAsyncConfigCallback;
	configureAllConfigSettingsAsyncConfigCallback = nullptr;
	byte userData = configureAllConfigSettingsAsyncConfigUserData;
	configureAllConfigSettingsAsyncConfigActive = false;

	DEBUG_PRINT_MILLIS
		DEBUG_PRINT("SetConfigDataAsync complete. Result: ");
	DEBUG_PRINTLN(result);

	if (cb) {
		cb(this, result, userData);
	}

}

void LD2410Async::configureAllConfigSettingsAsyncConfigModeDisabledCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Warning: Disabling config mode after configureAllConfigSettingsAsync failed. Result: ");
		DEBUG_PRINTLN(result);
		sender->configureAllConfigSettingsAsyncExecuteCallback(result);
	}
	else {
		// Config mode disabled successfully
		sender->configureAllConfigSettingsAsyncExecuteCallback(sender->configureAllConfigSettingsAsyncResultToReport);
	}
}

void LD2410Async::configureAllConfigSettingsAsyncFinalize(LD2410Async::AsyncCommandResult resultToReport) {

	if (configureAllConfigSettingsAsyncConfigInitialConfigMode) {
		configureAllConfigSettingsAsyncResultToReport = resultToReport;

		if (!disableConfigModeAsync(configureAllConfigSettingsAsyncConfigModeDisabledCallback, 0)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Disabling config mode after configureAllConfigSettingsAsync failed.");
			configureAllConfigSettingsAsyncExecuteCallback(LD2410Async::AsyncCommandResult::FAILED);
		}
	}
	else {
		configureAllConfigSettingsAsyncExecuteCallback(resultToReport);
	}
}


void LD2410Async::configureAllConfigSettingsAsyncWriteConfigCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (AsyncCommandResult::SUCCESS != result) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Writing config data to sensor failed.");
	};
	//Pass result to finalize method, which will also disable config mode if needed
	sender->configureAllConfigSettingsAsyncFinalize(result);
}


bool LD2410Async::configureAllConfigSettingsAsyncBuildSaveChangesCommandSequence() {
	//Get a clone of the current config, so it does not get changed while we are working
	LD2410Types::ConfigData currentConfig = LD2410Async::getConfigData();

	if (!resetCommandSequence()) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Could not reset command sequence.");
		return false;
	};
	// 1. Max gate + no one timeout
	if (configureAllConfigSettingsAsyncWriteFullConfig
		|| currentConfig.maxMotionDistanceGate != configureAllConfigSettingsAsyncConfigDataToWrite.maxMotionDistanceGate
		|| currentConfig.maxStationaryDistanceGate != configureAllConfigSettingsAsyncConfigDataToWrite.maxStationaryDistanceGate
		|| currentConfig.noOneTimeout != configureAllConfigSettingsAsyncConfigDataToWrite.noOneTimeout) {
		byte cmd[sizeof(LD2410Defs::maxGateCommandData)];
		if (!LD2410CommandBuilder::buildMaxGateCommand(cmd,
			configureAllConfigSettingsAsyncConfigDataToWrite.maxMotionDistanceGate,
			configureAllConfigSettingsAsyncConfigDataToWrite.maxStationaryDistanceGate,
			configureAllConfigSettingsAsyncConfigDataToWrite.noOneTimeout)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Building max gate command failed.");
			return false;
		};
		if (!addCommandToSequence(cmd)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Adding max gate command to sequence failed.");
			return false;
		};
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Max gate command added to sequence.");
	}
	// 2. Gate sensitivities (sequence of commands)
	for (byte gate = 0; gate < 9; gate++) {
		if (configureAllConfigSettingsAsyncWriteFullConfig
			|| currentConfig.distanceGateMotionSensitivity[gate] != configureAllConfigSettingsAsyncConfigDataToWrite.distanceGateMotionSensitivity[gate]
			|| currentConfig.distanceGateStationarySensitivity[gate] != configureAllConfigSettingsAsyncConfigDataToWrite.distanceGateStationarySensitivity[gate]) {
			byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
			if (!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate,
				configureAllConfigSettingsAsyncConfigDataToWrite.distanceGateMotionSensitivity[gate],
				configureAllConfigSettingsAsyncConfigDataToWrite.distanceGateStationarySensitivity[gate])) {
				DEBUG_PRINT_MILLIS;
				DEBUG_PRINT("Error: Error building gate sensitivity command for gate ");
				DEBUG_PRINTLN(gate);

				return false;
			};
			if (!addCommandToSequence(cmd)) {
				DEBUG_PRINT_MILLIS;
				DEBUG_PRINT("Error: Adding gate sensitivity command for gate ");
				DEBUG_PRINT(gate);
				DEBUG_PRINTLN(" to sequence failed.");

				return false;
			}
			DEBUG_PRINT_MILLIS
				DEBUG_PRINT("Gate sensitivity command for gate ");
			DEBUG_PRINT(gate);
			DEBUG_PRINTLN(" added to sequence.");
		}
	}

	//3. Distance resolution
	if (configureAllConfigSettingsAsyncWriteFullConfig
		|| currentConfig.distanceResolution != configureAllConfigSettingsAsyncConfigDataToWrite.distanceResolution) {
		byte cmd[6]; // resolution commands are 6 bytes long
		if (!LD2410CommandBuilder::buildDistanceResolutionCommand(cmd, configureAllConfigSettingsAsyncConfigDataToWrite.distanceResolution)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Building distance resolution command failed.");
			return false;
		};
		if (!addCommandToSequence(cmd)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Adding distance resolution command to sequence failed.");
			return false;
		};
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Distance resolution command added to sequence.");
	};

	//4. Aux control settings
	if (configureAllConfigSettingsAsyncWriteFullConfig
		|| currentConfig.lightControl != configureAllConfigSettingsAsyncConfigDataToWrite.lightControl
		|| currentConfig.lightThreshold != configureAllConfigSettingsAsyncConfigDataToWrite.lightThreshold
		|| currentConfig.outputControl != configureAllConfigSettingsAsyncConfigDataToWrite.outputControl) {
		byte cmd[sizeof(LD2410Defs::setAuxControlSettingCommandData)];
		if (!LD2410CommandBuilder::buildAuxControlCommand(cmd,
			configureAllConfigSettingsAsyncConfigDataToWrite.lightControl,
			configureAllConfigSettingsAsyncConfigDataToWrite.lightThreshold,
			configureAllConfigSettingsAsyncConfigDataToWrite.outputControl)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Building aux control command failed.");
			return false;
		};
		if (!addCommandToSequence(cmd)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Adding aux control command to sequence failed.");
			return false;
		};
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Aux control command added to sequence.");
	};
	return true;
};

bool LD2410Async::configureAllConfigSettingsAsyncWriteConfig() {

	if (!configureAllConfigSettingsAsyncBuildSaveChangesCommandSequence()) {
		//Could not build command sequence
		return false;
	}

	if (commandSequenceBufferCount == 0) {
		DEBUG_PRINT_MILLIS
			DEBUG_PRINTLN("No config changes detected, not need to write anything");
	}

	if (!executeCommandSequenceAsync(configureAllConfigSettingsAsyncWriteConfigCallback, 0)) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Starting command sequence to write config data failed.");

		return false;
	}
	return true;

};

void LD2410Async::configureAllConfigSettingsAsyncRequestAllConfigDataCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Requesting current config data failed. Result: ");
		DEBUG_PRINTLN(result);
		sender->configureAllConfigSettingsAsyncFinalize(result);
		return;
	}
	//Got current config data, now write the changed values
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Current config data received.");

	if (!sender->configureAllConfigSettingsAsyncWriteConfig()) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Starting saving config data changes failed.");
		sender->configureAllConfigSettingsAsyncFinalize(AsyncCommandResult::FAILED);
	}
}

bool LD2410Async::configureAllConfigSettingsAsyncRequestAllConfigData() {
	if (resetCommandSequence()
		&& addCommandToSequence(LD2410Defs::requestDistanceResolutionCommandData)
		&& addCommandToSequence(LD2410Defs::requestParamsCommandData)
		&& addCommandToSequence(LD2410Defs::requestAuxControlSettingsCommandData))
	{
		if (executeCommandSequenceAsync(configureAllConfigSettingsAsyncRequestAllConfigDataCallback, 0)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Requesting current config data");
			return true;
		}
		else {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Stating command sequence to request current config data failed.");
		}
	}
	return false;
}

void LD2410Async::configureAllConfigSettingsAsyncConfigModeEnabledCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result != AsyncCommandResult::SUCCESS) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Enabling config mode failed. Result: ");
		DEBUG_PRINTLN(result);
		sender->configureAllConfigSettingsAsyncFinalize(result);
		return;
	};

	//Got ack of enable config mode command
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Config mode enabled.");

	bool ret = false;
	if (sender->configureAllConfigSettingsAsyncWriteFullConfig) {
		//If we save all changes anyway, no need to request current config data first
		ret = sender->configureAllConfigSettingsAsyncWriteConfig();

	}
	else {
		ret = sender->configureAllConfigSettingsAsyncRequestAllConfigData();
	}
	if (!ret) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Starting config data write or request of current config data failed.");
		sender->configureAllConfigSettingsAsyncFinalize(AsyncCommandResult::FAILED);
	}


}

bool LD2410Async::configureAllConfigSettingsAsync(const LD2410Types::ConfigData& configToWrite, bool writeAllConfigData, AsyncCommandCallback callback, byte userData)
{

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Writing config data to the LD2410");

	if (asyncIsBusy()) return false;


	if (!configToWrite.isValid()) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("configToWrite is invalid.");
		return false;
	}

	configureAllConfigSettingsAsyncConfigActive = true;
	configureAllConfigSettingsAsyncConfigDataToWrite = configToWrite;
	configureAllConfigSettingsAsyncWriteFullConfig = writeAllConfigData;
	configureAllConfigSettingsAsyncConfigCallback = callback;
	configureAllConfigSettingsAsyncConfigUserData = userData;
	configureAllConfigSettingsAsyncConfigInitialConfigMode = isConfigModeEnabled();

	if (!configureAllConfigSettingsAsyncConfigInitialConfigMode) {
		return enableConfigModeInternalAsync(configureAllConfigSettingsAsyncConfigModeEnabledCallback, 0);
	}
	else {
		if (configureAllConfigSettingsAsyncWriteFullConfig) {
			//If we save all changes anyway, no need to request current config data first
			return configureAllConfigSettingsAsyncWriteConfig();
		}
		else {
			return configureAllConfigSettingsAsyncRequestAllConfigData();
		}
	}

}


/*--------------------------------------------------------------------
- Reboot command
---------------------------------------------------------------------*/
//The reboot command is special, since it needs to be sent in config mode,
//but doesnt have to disable config mode, since the sensor goes into normal
//detection mode after the reboot anyway.
void LD2410Async::rebootRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		sender->configModeEnabled = false;
		sender->engineeringModeEnabled = false;

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Reboot initiated");
	}
	else {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Error! Could not initiate reboot. Result: ");
		DEBUG_PRINTLN((int)result);
	}
	sender->executeCommandSequenceAsyncExecuteCallback(result);

}

void LD2410Async::rebootEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		//Got ack of enable config mode command
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Config mode enabled before reboot");
		sender->sendCommandAsync(LD2410Defs::rebootCommandData, rebootRebootCallback, 0);
	}
	else {
		//Config mode command timeout or canceled
		//Just execute the callback
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error! Could not enabled config mode before reboot");
		sender->executeCommandSequenceAsyncExecuteCallback(result);
	}
}


bool LD2410Async::rebootAsync(AsyncCommandCallback callback, byte userData) {


	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reboot");

	if (asyncIsBusy()) return false;

	return enableConfigModeInternalAsync(rebootEnableConfigModeCallback, 0);
}

/**********************************************************************************
* Data access
***********************************************************************************/
LD2410Types::DetectionData LD2410Async::getDetectionData() const {
	return detectionData;
}

LD2410Types::ConfigData LD2410Async::getConfigData() const {
	return configData;
}

/**********************************************************************************
* Inactivity handling
***********************************************************************************/
void LD2410Async::handleInactivityRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	sender->configModeEnabled = false;
	sender->engineeringModeEnabled = false;

#if (LD2410ASYNC_DEBUG_LEVEL > 0)
	if (result == AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("LD2410 reboot due to inactivity initiated");
	}
	else {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Error!! Could not initiate LD2410 reboot. Result: ");
		DEBUG_PRINTLN((int)result);
	}

#endif

}

void LD2410Async::handleInactivityDisableConfigmodeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {


#if (LD2410ASYNC_DEBUG_LEVEL > 0)
	if (result == AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Config mode disabled due to inactivity");
	}
	else {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Error!! Disabling config mode after inactivity failed. Result: ");
		DEBUG_PRINTLN((int)result);
	}

#endif

}


void LD2410Async::handleInactivity() {

	if (inactivityHandlingEnabled && inactivityHandlingTimeoutMs > 0) {
		unsigned long currentTime = millis();
		unsigned long inactiveDurationMs = currentTime - lastActivityMs;
		if (lastActivityMs != 0 && inactiveDurationMs > inactivityHandlingTimeoutMs) {
			if (!handleInactivityExitConfigModeDone) {
				handleInactivityExitConfigModeDone = true;
				disableConfigModeAsync(handleInactivityDisableConfigmodeCallback, 0);
			}
			else if (inactiveDurationMs > inactivityHandlingTimeoutMs + 5000) {
				rebootAsync(handleInactivityRebootCallback, 0);
				lastActivityMs = currentTime;
			}
		}
		else {
			handleInactivityExitConfigModeDone = false;
		}
	}
}

void LD2410Async::heartbeat() {
	lastActivityMs = millis();
}

void LD2410Async::setInactivityHandling(bool enable) {
	inactivityHandlingEnabled = enable;
}

/**********************************************************************************
* Process received data
***********************************************************************************/
void LD2410Async::processReceivedData()
{
	FrameReadResponse type = readFrame();

	switch (type) {
	case ACK:
		processAck();
		break;
	case DATA:
		processData();
		break;
	default:
		break;
	}


}

/**********************************************************************************
* Task methods
***********************************************************************************/

void LD2410Async::taskLoop() {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Task loop starts");

	while (!taskStop) {
		processReceivedData();
		handleAsyncCommandCallbackTimeout();
		handleInactivity();
		vTaskDelay(10 / portTICK_PERIOD_MS);

	}

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINT(taskStop);
	DEBUG_PRINTLN("Task loop exits");

	// Signal that the task is done
	taskHandle = NULL;
	vTaskDelete(NULL);  // self-delete
}



/**********************************************************************************
* Begin, end
***********************************************************************************/

bool LD2410Async::begin() {
	if (taskHandle == NULL) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Starting data processing task");
		taskStop = false;

		BaseType_t result = xTaskCreate(
			[](void* param) {
				if (param) {
					static_cast<LD2410Async*>(param)->taskLoop();
				}
				vTaskDelete(NULL);
			},
			"LD2410Task",
			4096,
			this,
			1,
			&taskHandle
		);

		if (result == pdPASS) {
			return true;
		}
		else {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Task creation failed");
			taskHandle = NULL;
			return false;
		}
	}
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Data processing task already active");
	return false;
}

bool LD2410Async::end() {
	if (taskHandle != NULL) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Stopping data processing task");
		taskStop = true;

		// Wait up to 200ms for graceful exit
		for (int i = 0; i < 20; i++) {
			if (taskHandle == NULL) {
				DEBUG_PRINT_MILLIS;
				DEBUG_PRINTLN("Task exited gracefully");
				return true;
			}
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}

		// If still not NULL, force delete
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Forcing task stop");
		vTaskDelete(taskHandle);
		taskHandle = NULL;
		return true;
	}

	DEBUG_PRINTLN("Data processing task is not active");
	return false;
}




/**********************************************************************************
* Constructor
***********************************************************************************/
LD2410Async::LD2410Async(Stream& serial)
{
	sensor = &serial;
}
