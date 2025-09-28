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


void printBuf(const byte* buf, byte size)
{
	for (byte i = 0; i < size; i++)
	{
		Serial.print(byte2hex(buf[i]));
		Serial.print(' ');
	}
	Serial.println();
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
		protocolVersion = receiveBuffer[4] | (receiveBuffer[5] << 8);
		bufferSize = receiveBuffer[6] | (receiveBuffer[7] << 8);
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
			mac[i] = receiveBuffer[i + 4];
		};
		macString = byte2hex(mac[0])
			+ ":" + byte2hex(mac[1])
			+ ":" + byte2hex(mac[2])
			+ ":" + byte2hex(mac[3])
			+ ":" + byte2hex(mac[4])
			+ ":" + byte2hex(mac[5]);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ACK for requestBluetoothMacAddressAsyncCommand received");
		break;
	case LD2410Defs::requestFirmwareCommand:
		firmware = byte2hex(receiveBuffer[7], false)
			+ "." + byte2hex(receiveBuffer[6])
			+ "." + byte2hex(receiveBuffer[11]) + byte2hex(receiveBuffer[10]) + byte2hex(receiveBuffer[9]) + byte2hex(receiveBuffer[8]);
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

	//Dont check with asyncIsBusy() since this would also checks if sendConfigCommandAsync has started a command which is using this method and asyncIsBusy would therefore return true which would block the command.
	if (asyncCommandCallback == nullptr) {


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
		DEBUG_PRINT("Error! Did not send async command ");
		DEBUG_PRINTLN(byte2hex(command[2]));

		return false;
	}


}
/**********************************************************************************
* Async command busy
***********************************************************************************/

bool LD2410Async::asyncIsBusy() {
	return asyncCommandActive || sendAsyncSequenceActive;
}

/**********************************************************************************
* Send async config command methods
***********************************************************************************/


bool LD2410Async::sendConfigCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;

	// Reset sequence buffer
	if (!resetCommandSequence()) return false;;

	// Add the single command
	if (!addCommandToSequence(command)) return false;

	// Execute as a sequence (with just one command)
	return sendCommandSequenceAsync(callback, userData);
}

/**********************************************************************************
* Async command sequence methods
***********************************************************************************/
void LD2410Async::executeAsyncSequenceCallback(LD2410Async::AsyncCommandResult result) {
	if (sendAsyncSequenceActive) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Command sequence duration ms: ");
		DEBUG_PRINTLN(millis() - sendAsyncSequenceStartMs);

		vTaskSuspendAll();
		AsyncCommandCallback cb = sendAsyncSequenceCallback;
		byte userData = sendAsyncSequenceUserData;
		sendAsyncSequenceCallback = nullptr;
		sendAsyncSequenceUserData = 0;
		sendAsyncSequenceActive = false;
		xTaskResumeAll();

		if (cb != nullptr) {
			cb(this, result, userData);
		}
	}
}


// Callback after disabling config mode at the end of sequence
void LD2410Async::sendCommandSequenceAsyncDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {

	LD2410Async::AsyncCommandResult sequenceResult = static_cast<LD2410Async::AsyncCommandResult>(userData);

	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Warning: Disabling config mode after command sequence failed. Result: ");
		DEBUG_PRINTLN(result);
		sequenceResult = result; // report disable failure if it happened
	}
	sender->executeAsyncSequenceCallback(sequenceResult);
}


// Called when a single command in the sequence completes
void LD2410Async::sendCommandSequenceAsyncCommandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		// Abort sequence if a command fails
		DEBUG_PRINT_MILLIS
			DEBUG_PRINT("Error: Command sequence aborted due to command failure. Result: ");
		DEBUG_PRINTLN(result);
		if (!sender->sendAsyncSequenceInitialConfigModeState) {
			sender->disableConfigModeAsync(sendCommandSequenceAsyncDisableConfigModeCallback, static_cast<byte>(result));
		}
		else {
			sender->executeAsyncSequenceCallback(result);
		}
		return;
	}

	// Move to next command
	sender->sendAsyncSequenceIndex++;

	if (sender->sendAsyncSequenceIndex < sender->commandSequenceBufferCount) {
		// Send next command
		sender->sendCommandAsync(
			sender->commandSequenceBuffer[sender->sendAsyncSequenceIndex],
			sendCommandSequenceAsyncCommandCallback,
			0
		);
	}
	else {
		// Sequence finished successfully
		if (!sender->sendAsyncSequenceInitialConfigModeState) {
			sender->disableConfigModeAsync(sendCommandSequenceAsyncDisableConfigModeCallback, static_cast<byte>(LD2410Async::AsyncCommandResult::SUCCESS));
		}
		else {
			sender->executeAsyncSequenceCallback(LD2410Async::AsyncCommandResult::SUCCESS);
		}
	}
}


// After enabling config mode at the beginning
void LD2410Async::sendCommandSequenceAsyncEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		// Start with first command
		sender->sendAsyncSequenceIndex = 0;
		sender->sendCommandAsync(
			sender->commandSequenceBuffer[sender->sendAsyncSequenceIndex],
			sendCommandSequenceAsyncCommandCallback,
			0
		);
	}
	else {
		// Failed to enable config mode
		DEBUG_PRINT_MILLIS
			DEBUG_PRINT("Error: Enabling config mode before command sequence failed. Result:");
		DEBUG_PRINTLN(result);
		sender->executeAsyncSequenceCallback(result);
	}
}




bool LD2410Async::sendCommandSequenceAsync(AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;
	if (commandSequenceBufferCount == 0) return true; // nothing to send

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINT("Starting command sequence execution. Number of commands: ");
	DEBUG_PRINTLN(commandSequenceBufferCount);

	sendAsyncSequenceActive = true;
	sendAsyncSequenceCallback = callback;
	sendAsyncSequenceUserData = userData;
	sendAsyncSequenceInitialConfigModeState = configModeEnabled;
	sendAsyncSequenceIndex = 0;
	sendAsyncSequenceStartMs = millis();

	if (!configModeEnabled) {
		enableConfigModeAsync(sendCommandSequenceAsyncEnableConfigModeCallback, 0);
	}
	else {
		// Already in config mode, start directly
		sendCommandAsync(
			commandSequenceBuffer[sendAsyncSequenceIndex],
			sendCommandSequenceAsyncCommandCallback,
			0
		);
	}

	return true;
}

bool LD2410Async::addCommandToSequence(const byte* command) {
	if (asyncIsBusy()) return false;

	if (commandSequenceBufferCount >= MAX_COMMAND_SEQUENCE) return false; // buffer full

	// First byte of the command is the payload length
	uint8_t len = command[0];
	uint8_t totalLen = len + 2; // payload + 2 length bytes

	if (totalLen > LD2410Defs::LD2410_Buffer_Size) return false; // safety

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
	if (asyncIsBusy()) return false;
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Command sequence reset done.");

	commandSequenceBufferCount = 0;
	return true;
}

/**********************************************************************************
* Async commands methods
***********************************************************************************/
bool LD2410Async::enableConfigModeAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable Config Mode");
	return sendCommandAsync(LD2410Defs::configEnableCommandData, callback, userData);
}

bool LD2410Async::disableConfigModeAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable Config Mode");
	return sendCommandAsync(LD2410Defs::configDisableCommandData, callback, userData);
}


bool LD2410Async::setMaxGateAndNoOneTimeoutAsync(byte maxMovingGate, byte maxStationaryGate,
	unsigned short noOneTimeout,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Max Gate");

	byte cmd[sizeof(LD2410Defs::maxGateCommandData)];
	if (LD2410CommandBuilder::buildMaxGateCommand(cmd, maxMovingGate, maxStationaryGate, noOneTimeout)) {


		return sendConfigCommandAsync(cmd, callback, userData);
	}
	return false;
}


bool LD2410Async::requestGateParametersAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Gate Parameters");
	return sendConfigCommandAsync(LD2410Defs::requestParamsCommandData, callback, userData);
}

bool LD2410Async::enableEngineeringModeAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable EngineeringMode");
	return sendConfigCommandAsync(LD2410Defs::engineeringModeEnableCommandData, callback, userData);
}

bool LD2410Async::disableEngineeringModeAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable EngineeringMode");
	return sendConfigCommandAsync(LD2410Defs::engineeringModeDisableCommandData, callback, userData);
}

bool LD2410Async::setDistanceGateSensitivityAsync(const byte movingThresholds[9],
	const byte stationaryThresholds[9],
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Gate Sensitivities (all gates)");

	if (!resetCommandSequence()) return false;

	for (byte gate = 0; gate < 9; gate++) {
		byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
		if (!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate, movingThresholds[gate], stationaryThresholds[gate])) return false;
		if (!addCommandToSequence(cmd)) return false;
	}

	return sendCommandSequenceAsync(callback, userData);
}



bool LD2410Async::setDistanceGateSensitivityAsync(byte gate, byte movingThreshold,
	byte stationaryThreshold,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Gate Sensitivity");

	byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
	if(!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate, movingThreshold, stationaryThreshold)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::requestFirmwareAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Firmware");
	return sendConfigCommandAsync(LD2410Defs::requestFirmwareCommandData, callback, userData);
}


bool LD2410Async::setBaudRateAsync(byte baudRateSetting,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Baud Rate");

	if ((baudRateSetting < 1) || (baudRateSetting > 8))
		return false;

	byte cmd[sizeof(LD2410Defs::setBaudRateCommandData)];
	if(!LD2410CommandBuilder::buildBaudRateCommand(cmd, baudRateSetting)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::setBaudRateAsync(LD2410Types::Baudrate baudRate, AsyncCommandCallback callback, byte userData) {
	return setBaudRateAsync((byte)baudRate, callback, userData);
}


bool LD2410Async::restoreFactorySettingsAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Restore Factory Settings");
	return sendConfigCommandAsync(LD2410Defs::restoreFactorSettingsCommandData, callback, userData);
}



bool LD2410Async::enableBluetoothAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable Bluetooth");
	return sendConfigCommandAsync(LD2410Defs::bluetoothSettingsOnCommandData, callback, userData);
}

bool LD2410Async::disableBluetoothAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable Bluetooth");
	return sendConfigCommandAsync(LD2410Defs::bluetoothSettingsOnCommandData, callback, userData);
}


bool LD2410Async::requestBluetoothMacAddressAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Mac Address");
	return sendConfigCommandAsync(LD2410Defs::requestMacAddressCommandData, callback, userData);
}

bool LD2410Async::setBluetoothpasswordAsync(const char* password,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Bluetooth Password");

	byte cmd[sizeof(LD2410Defs::setBluetoothPasswordCommandData)];
	if (!LD2410CommandBuilder::buildBluetoothPasswordCommand(cmd, password)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::setBluetoothpasswordAsync(const String& password, AsyncCommandCallback callback, byte userData) {

	return setBluetoothpasswordAsync(password.c_str(), callback, userData);
}


bool LD2410Async::resetBluetoothpasswordAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reset Bluetooth Password");
	return sendConfigCommandAsync(LD2410Defs::setBluetoothPasswordCommandData, callback, userData);
}

bool LD2410Async::setDistanceResolution75cmAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution 75cm");
	return sendConfigCommandAsync(LD2410Defs::setDistanceResolution75cmCommandData, callback, userData);
};

bool LD2410Async::setDistanceResolutionAsync(LD2410Types::DistanceResolution distanceResolution,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution");

	byte cmd[6];
	if(!LD2410CommandBuilder::buildDistanceResolutionCommand(cmd, distanceResolution)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}

bool LD2410Async::setDistanceResolution20cmAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution 20cm");
	return sendConfigCommandAsync(LD2410Defs::setDistanceResolution20cmCommandData, callback, userData);
};

bool LD2410Async::requestDistanceResolutioncmAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Distance Resolution cm");
	return sendConfigCommandAsync(LD2410Defs::requestDistanceResolutionCommandData, callback, userData);
}

bool LD2410Async::setAuxControlSettingsAsync(LD2410Types::LightControl lightControl, byte lightThreshold,
	LD2410Types::OutputControl outputControl,
	AsyncCommandCallback callback, byte userData)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Aux Control Settings");



	byte cmd[sizeof(LD2410Defs::setAuxControlSettingCommandData)];
	if (!LD2410CommandBuilder::buildAuxControlCommand(cmd, lightControl, lightThreshold, outputControl)) return false;

	return sendConfigCommandAsync(cmd, callback, userData);
}



bool LD2410Async::requestAuxControlSettingsAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Aux Control Settings");
	return sendConfigCommandAsync(LD2410Defs::requestAuxControlSettingsCommandData, callback, userData);
}


bool LD2410Async::beginAutoConfigAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Begin auto config");
	return sendConfigCommandAsync(LD2410Defs::beginAutoConfigCommandData, callback, userData);
};

bool LD2410Async::requestAutoConfigStatusAsync(AsyncCommandCallback callback, byte userData) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reqtest auto config status");
	return sendConfigCommandAsync(LD2410Defs::requestAutoConfigStatusCommandData, callback, userData);
}


bool LD2410Async::requestAllStaticData(AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request all static data");

	if (!resetCommandSequence()) return false;

	if (!addCommandToSequence(LD2410Defs::requestFirmwareCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestMacAddressCommandData)) return false;

	return sendCommandSequenceAsync(callback, userData);
}

bool LD2410Async::requestAllConfigDataAsync(AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request all config data");

	if (!resetCommandSequence()) return false;
	if (!addCommandToSequence(LD2410Defs::requestDistanceResolutionCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestParamsCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestAuxControlSettingsCommandData)) return false;

	return sendCommandSequenceAsync(callback, userData);

}

bool LD2410Async::setConfigDataAsync(const LD2410Types::ConfigData& config, AsyncCommandCallback callback, byte userData)
{
	if (asyncIsBusy()) return false;

	if (!config.isValid()) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("ConfigData invalid");
		return false;
	}

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set full ConfigData");

	if (!resetCommandSequence()) return false;

	// 1. Max gate + no one timeout
	{
		byte cmd[sizeof(LD2410Defs::maxGateCommandData)];
		if (!LD2410CommandBuilder::buildMaxGateCommand(cmd, config.maxMotionDistanceGate, config.maxStationaryDistanceGate, config.noOneTimeout)) return false;
		if (!addCommandToSequence(cmd)) return false;
	}

	// 2. Gate sensitivities (sequence of commands)
	for (byte gate = 0; gate < 9; gate++) {
		byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
		if (!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate, config.distanceGateMotionSensitivity[gate], config.distanceGateStationarySensitivity[gate])) return false;
		if (!addCommandToSequence(cmd)) return false;
	}

	// 3. Distance resolution
	{
		byte cmd[6]; // resolution commands are 6 bytes long
		if (!LD2410CommandBuilder::buildDistanceResolutionCommand(cmd, config.distanceResolution)) return false;
		if (!addCommandToSequence(cmd)) return false;
	}

	// 4. Aux control settings
	{
		byte cmd[sizeof(LD2410Defs::setAuxControlSettingCommandData)];
		if (!LD2410CommandBuilder::buildAuxControlCommand(cmd, config.lightControl, config.lightThreshold, config.outputControl)) return false;
		if (!addCommandToSequence(cmd)) return false;
	}

	// Execute the sequence
	return sendCommandSequenceAsync(callback, userData);
}





/*--------------------------------------------------------------------
- Reboot command
---------------------------------------------------------------------*/

void LD2410Async::rebootRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		sender->configModeEnabled = false;
		sender->engineeringModeEnabled = false;

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Reboot initiated");
	}
	sender->executeAsyncSequenceCallback(result);

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
		sender->executeAsyncSequenceCallback(result);
	}
}


bool LD2410Async::rebootAsync(AsyncCommandCallback callback, byte userData) {
	if (asyncIsBusy()) return false;

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reboot");

	sendAsyncSequenceCallback = callback;
	sendAsyncSequenceUserData = userData;


	return enableConfigModeAsync(rebootEnableConfigModeCallback, 0);
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

#ifdef ENABLE_DEBUG
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


#ifdef ENABLE_DEBUG
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
