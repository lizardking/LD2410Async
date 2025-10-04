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
			//Invalid payload size, wait for header.

			DEBUG_PRINT("Invalid payload size read: ");
			DEBUG_PRINTLN(payloadSize);

			readFrameState = ReadFrameState::WAITING_FOR_HEADER;
		}
		else {




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


			return succesResponseType;
		}
		else {
			DEBUG_PRINTLN(" Invalid frame end: ");
			DEBUG_PRINTBUF(receiveBuffer, receiveBufferIndex);

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
void LD2410Async::onDetectionDataReceived(DetectionDataCallback callback) {
	detectionDataCallback = callback;
}

void LD2410Async::onConfigDataReceived(GenericCallback callback) {
	configUpdateReceivedReceivedCallback = callback;
}

void LD2410Async::onConfigChanged(GenericCallback callback) {
	configChangedCallback = callback;
}



void LD2410Async::executeConfigUpdateReceivedCallback() {

	if (configUpdateReceivedReceivedCallback) {
		configUpdateReceivedReceivedCallback(this);
	}

}

void LD2410Async::executeConfigChangedCallback() {

	if (configChangedCallback) {
		configChangedCallback(this);
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
		sendCommandAsyncExecuteCallback(command, LD2410Async::AsyncCommandResult::FAILED);
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
		sendCommandAsyncExecuteCallback(command, LD2410Async::AsyncCommandResult::SUCCESS);
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

		if (rebootAsyncPending) {
			rebootAsyncFinialize(LD2410Async::AsyncCommandResult::SUCCESS);
		}

		if (detectionDataCallback != nullptr) {
			detectionDataCallback(this, detectionData.presenceDetected);
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

}

/**********************************************************************************
* Send async command methods
***********************************************************************************/
void LD2410Async::sendCommandAsyncExecuteCallback(byte commandCode, LD2410Async::AsyncCommandResult result) {
	if (sendCommandAsyncCommandPending && sendCommandAsyncCommandCode == commandCode) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Async command duration ms: ");
		DEBUG_PRINTLN(millis() - sendCommandAsyncStartMs);

		//Just to be sure that no other task changes the callback data or registers a callback before the callback has been executed
		vTaskSuspendAll();
		AsyncCommandCallback cb = sendCommandAsyncCallback;
		sendCommandAsyncCallback = nullptr;
		sendCommandAsyncStartMs = 0;
		sendCommandAsyncCommandCode = 0;
		sendCommandAsyncCommandPending = false;
		xTaskResumeAll();

		if (cb != nullptr) {
			cb(this, result);
		}
	}
}




void LD2410Async::asyncCancel() {
	//Will also trigger the callback on command sequences
	sendCommandAsyncExecuteCallback(sendCommandAsyncCommandCode, LD2410Async::AsyncCommandResult::CANCELED);

	//Just in case reboot is still waiting 
	//since there might not be an active command or command sequence while we wait for normal operation.
	rebootAsyncFinialize(LD2410Async::AsyncCommandResult::CANCELED);
}

void LD2410Async::sendCommandAsyncHandleTimeout() {

	if (sendCommandAsyncCommandPending && sendCommandAsyncStartMs != 0) {
		unsigned long elapsedTime = millis() - sendCommandAsyncStartMs;
		if (elapsedTime > asyncCommandTimeoutMs) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINT("Command timeout detected. Elapsed time: ");
			DEBUG_PRINT(elapsedTime);
			DEBUG_PRINTLN("ms.");

			if (sendCommandAsyncRetriesLeft > 0) {
				DEBUG_PRINT("Retries left: ");
				DEBUG_PRINTLN(sendCommandAsyncRetriesLeft);
				sendCommandAsyncRetriesLeft--;

				sendCommandAsyncStartMs = millis();
				sendCommand(sendCommandAsyncCommandBuffer);
			}
			else {

				DEBUG_PRINTLN("Execute callback with TIMEOUT result.");
				sendCommandAsyncExecuteCallback(sendCommandAsyncCommandCode, LD2410Async::AsyncCommandResult::TIMEOUT);
			}
		}
	}
}



void LD2410Async::sendCommandAsyncStoreDataForCallback(const byte* command, byte retries, AsyncCommandCallback callback) {
	sendCommandAsyncCommandPending = true;
	sendCommandAsyncCallback = callback;
	sendCommandAsyncCommandCode = command[2];
	sendCommandAsyncRetriesLeft = retries;
	if (retries > 0) {	//No need to copy the data if we are not going to retry anyway
		memcpy(sendCommandAsyncCommandBuffer, command, command[0] + 2);
	}
}

bool LD2410Async::sendCommandAsync(const byte* command, byte retries, AsyncCommandCallback callback)
{
	vTaskSuspendAll();

	//Dont check with asyncIsBusy() since this would also check if a sequence or setConfigDataASync is active
	if (!sendCommandAsyncCommandPending) {


		//Register data for callback
		sendCommandAsyncStoreDataForCallback(command, retries, callback);
		xTaskResumeAll();

		sendCommandAsyncStartMs = millis();
		sendCommand(command);
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Async command ");
		DEBUG_PRINT(byte2hex(sendCommandAsyncCommandCode));
		DEBUG_PRINTLN(" sent");

		return true;
	}
	else {
		xTaskResumeAll();
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Error! Async command is pending. Did not send async command ");
		DEBUG_PRINTLN(byte2hex(command[2]));

		return false;
	}


}
/**********************************************************************************
* Async command busy
***********************************************************************************/

bool LD2410Async::asyncIsBusy() {
	return sendCommandAsyncCommandPending || executeCommandSequencePending || configureAllConfigSettingsAsyncConfigActive || rebootAsyncPending;
}

/**********************************************************************************
* Send async config command methods
***********************************************************************************/


bool LD2410Async::sendConfigCommandAsync(const byte* command, AsyncCommandCallback callback) {
	//Dont check with asyncIsBusy() since this would also check if a sequence or setConfigDataASync is active
	if (sendCommandAsyncCommandPending || executeCommandSequencePending) return false;

	// Reset sequence buffer
	if (!resetCommandSequence()) return false;;

	// Add the single command
	if (!addCommandToSequence(command)) return false;

	// Execute as a sequence (with just one command)
	return executeCommandSequenceAsync(callback);
}

/**********************************************************************************
* Async command sequence methods
***********************************************************************************/
void LD2410Async::executeCommandSequenceAsyncExecuteCallback(LD2410Async::AsyncCommandResult result) {
	if (executeCommandSequencePending) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Command sequence duration ms: ");
		DEBUG_PRINTLN(millis() - executeCommandSequenceStartMs);

		vTaskSuspendAll();
		AsyncCommandCallback cb = executeCommandSequenceCallback;
		executeCommandSequenceCallback = nullptr;
		executeCommandSequencePending = false;
		xTaskResumeAll();

		if (cb != nullptr) {
			cb(this, result);
		}
	}
}


// Callback after disabling config mode at the end of sequence
void LD2410Async::executeCommandSequenceAsyncDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {

	LD2410Async::AsyncCommandResult sequenceResult = sender->executeCommandSequenceResultToReport;

	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Warning: Disabling config mode after command sequence failed. Result: ");
		DEBUG_PRINTLN((byte)result);
		sequenceResult = result; // report disable failure if it happened
	}
	sender->executeCommandSequenceAsyncExecuteCallback(sequenceResult);
}

void LD2410Async::executeCommandSequenceAsyncFinalize(LD2410Async::AsyncCommandResult result) {
	if (!executeCommandSequenceInitialConfigModeState) {
		executeCommandSequenceResultToReport = result;
		if (!disableConfigModeInternalAsync(executeCommandSequenceAsyncDisableConfigModeCallback)) {
			executeCommandSequenceAsyncExecuteCallback(LD2410Async::AsyncCommandResult::FAILED);
		}
	}
	else {
		executeCommandSequenceAsyncExecuteCallback(result);
	}
}


// Called when a single command in the sequence completes
void LD2410Async::executeCommandSequenceAsyncCommandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		// Abort sequence if a command fails
		DEBUG_PRINT_MILLIS
			DEBUG_PRINT("Error: Command sequence aborted due to command failure. Result: ");
		DEBUG_PRINTLN((byte)result);
		sender->executeCommandSequenceAsyncFinalize(result);

		return;
	};

	// Move to next command
	sender->executeCommandSequenceIndex++;

	if (sender->executeCommandSequenceIndex < sender->commandSequenceBufferCount) {
		// Send next command
		sender->sendCommandAsync(sender->commandSequenceBuffer[sender->executeCommandSequenceIndex], executeCommandSequenceAsyncCommandCallback);
	}
	else {
		// Sequence finished successfully

		sender->executeCommandSequenceAsyncFinalize(LD2410Async::AsyncCommandResult::SUCCESS);
	}
}



bool LD2410Async::executeCommandSequenceAsync(AsyncCommandCallback callback) {
	if (sendCommandAsyncCommandPending || executeCommandSequencePending) return false;
	if (commandSequenceBufferCount == 0) return true; // nothing to send

	DEBUG_PRINT_MILLIS;
	DEBUG_PRINT("Starting command sequence execution. Number of commands: ");
	DEBUG_PRINTLN(commandSequenceBufferCount);

	executeCommandSequencePending = true;
	executeCommandSequenceCallback = callback;
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
		return sendCommandAsync(LD2410Defs::configEnableCommandData, 1, executeCommandSequenceAsyncCommandCallback); //Retry once since this command will sometimes not work resp. not send an ack.
	}
	else {
		// Already in config mode, start directly. 
		// We start the first command in the sequence directly and set the sequence index 0, so the second command (if any) is executed when the callback fires. 
		executeCommandSequenceIndex = 0;
		return sendCommandAsync(commandSequenceBuffer[executeCommandSequenceIndex], executeCommandSequenceAsyncCommandCallback);
	}

}

bool LD2410Async::addCommandToSequence(const byte* command) {
	if (sendCommandAsyncCommandPending || executeCommandSequencePending) return false;

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
	if (sendCommandAsyncCommandPending || executeCommandSequencePending) return false;

	commandSequenceBufferCount = 0;
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Command sequence reset done.");

	return true;
}

/**********************************************************************************
* Config mode commands
***********************************************************************************/
//Config mode command have a internal version which does not check for busy and can therefore be used within other command implementations




bool LD2410Async::enableConfigModeInternalAsync(bool force, AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable Config Mode");

	if (!configModeEnabled || force) {
		// Need to send the enable command
		return sendCommandAsync(LD2410Defs::configEnableCommandData, 1, callback); //We do 1 retry if necessary, since this command does sometimes not work/resp not send a ack
	}
	else {
		// Already in config mode -> just trigger the callback after a tiny delay
		sendCommandAsyncStoreDataForCallback(LD2410Defs::configEnableCommandData, 0, callback);
		configModeOnceTicker.once_ms(1, [this]() {
			sendCommandAsyncExecuteCallback(LD2410Defs::configEnableCommand, LD2410Async::AsyncCommandResult::SUCCESS);
			});
		return true;
	}
}

bool LD2410Async::enableConfigModeAsync(bool force, AsyncCommandCallback callback) {
	if (asyncIsBusy()) return false;
	return enableConfigModeInternalAsync(force, callback);
}

bool LD2410Async::disableConfigModeInternalAsync(bool force, AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable Config Mode");

	if (configModeEnabled || force) {
		return sendCommandAsync(LD2410Defs::configDisableCommandData, callback);
	}
	else {
		//If config mode doesnt have to be disabled, the callback gets triggered after a minimal delay (to ensure the disableConfigMode method can complete before the callback fires.
		sendCommandAsyncStoreDataForCallback(LD2410Defs::configDisableCommandData, 0, callback);
		configModeOnceTicker.once_ms(1, [this]() {
			sendCommandAsyncExecuteCallback(LD2410Defs::configDisableCommand, LD2410Async::AsyncCommandResult::SUCCESS);
			});
		return true;
	}
}

bool LD2410Async::disableConfigModeAsync(bool force, AsyncCommandCallback callback) {
	if (asyncIsBusy()) return false;
	return disableConfigModeInternalAsync(force, callback);
}

/**********************************************************************************
* Native LD2410 commands to control, configure and query the sensor
***********************************************************************************/
// All these commands need to be executed in config mode.
// The code takes care of that. Enables/disable config mode if necessary, 
// but also keeps config mode active if it was already active before the command
bool LD2410Async::configureMaxGateAndNoOneTimeoutAsync(byte maxMovingGate, byte maxStationaryGate,
	unsigned short noOneTimeout,
	AsyncCommandCallback callback)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINT(noOneTimeout);
	DEBUG_PRINTLN("Set Max Gate");
	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::maxGateCommandData)];
	if (LD2410CommandBuilder::buildMaxGateCommand(cmd, maxMovingGate, maxStationaryGate, noOneTimeout)) {
		return sendConfigCommandAsync(cmd, callback);
	}
	return false;
}


bool LD2410Async::requestGateParametersAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Gate Parameters");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::requestParamsCommandData, callback);
}

bool LD2410Async::enableEngineeringModeAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable EngineeringMode");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::engineeringModeEnableCommandData, callback);
}

bool LD2410Async::disableEngineeringModeAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable EngineeringMode");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::engineeringModeDisableCommandData, callback);
}

bool LD2410Async::configureDistanceGateSensitivityAsync(const byte movingThresholds[9],
	const byte stationaryThresholds[9],
	AsyncCommandCallback callback)
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

	return executeCommandSequenceAsync(callback);
}



bool LD2410Async::configureDistanceGateSensitivityAsync(byte gate, byte movingThreshold,
	byte stationaryThreshold,
	AsyncCommandCallback callback)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Gate Sensitivity");

	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData)];
	if (!LD2410CommandBuilder::buildGateSensitivityCommand(cmd, gate, movingThreshold, stationaryThreshold)) return false;

	return sendConfigCommandAsync(cmd, callback);
}



bool LD2410Async::requestFirmwareAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Firmware");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::requestFirmwareCommandData, callback);
}


bool LD2410Async::configureBaudRateAsync(byte baudRateSetting,
	AsyncCommandCallback callback)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Baud Rate");

	if (asyncIsBusy()) return false;

	if ((baudRateSetting < 1) || (baudRateSetting > 8))
		return false;

	byte cmd[sizeof(LD2410Defs::setBaudRateCommandData)];
	if (!LD2410CommandBuilder::buildBaudRateCommand(cmd, baudRateSetting)) return false;

	return sendConfigCommandAsync(cmd, callback);
}



bool LD2410Async::configureBaudRateAsync(LD2410Types::Baudrate baudRate, AsyncCommandCallback callback) {

	if (asyncIsBusy()) return false;

	return configureBaudRateAsync((byte)baudRate, callback);
}


bool LD2410Async::restoreFactorySettingsAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Restore Factory Settings");

	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::restoreFactorSettingsCommandData, callback);
}



bool LD2410Async::enableBluetoothAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Enable Bluetooth");
	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::bluetoothSettingsOnCommandData, callback);
}

bool LD2410Async::disableBluetoothAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Disable Bluetooth");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::bluetoothSettingsOnCommandData, callback);
}


bool LD2410Async::requestBluetoothMacAddressAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Mac Address");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::requestMacAddressCommandData, callback);
}

bool LD2410Async::configureBluetoothPasswordAsync(const char* password,
	AsyncCommandCallback callback)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Bluetooth Password");
	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::setBluetoothPasswordCommandData)];
	if (!LD2410CommandBuilder::buildBluetoothPasswordCommand(cmd, password)) return false;

	return sendConfigCommandAsync(cmd, callback);
}



bool LD2410Async::configureBluetoothPasswordAsync(const String& password, AsyncCommandCallback callback) {

	return configureBluetoothPasswordAsync(password.c_str(), callback);
}


bool LD2410Async::configureDefaultBluetoothPasswordAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reset Bluetooth Password");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::setBluetoothPasswordCommandData, callback);
}

bool LD2410Async::configureDistanceResolution75cmAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution 75cm");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::setDistanceResolution75cmCommandData, callback);
};

bool LD2410Async::configureDistanceResolutionAsync(LD2410Types::DistanceResolution distanceResolution,
	AsyncCommandCallback callback)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution");
	if (asyncIsBusy()) return false;

	byte cmd[6];
	if (!LD2410CommandBuilder::buildDistanceResolutionCommand(cmd, distanceResolution)) return false;

	return sendConfigCommandAsync(cmd, callback);
}

bool LD2410Async::configuresDistanceResolution20cmAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Distance Resolution 20cm");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::setDistanceResolution20cmCommandData, callback);
};

bool LD2410Async::requestDistanceResolutionAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Distance Resolution cm");
	if (asyncIsBusy()) return false;
	return sendConfigCommandAsync(LD2410Defs::requestDistanceResolutionCommandData, callback);
}

bool LD2410Async::configureAuxControlSettingsAsync(LD2410Types::LightControl lightControl, byte lightThreshold,
	LD2410Types::OutputControl outputControl,
	AsyncCommandCallback callback)
{
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Set Aux Control Settings");
	if (asyncIsBusy()) return false;

	byte cmd[sizeof(LD2410Defs::setAuxControlSettingCommandData)];
	if (!LD2410CommandBuilder::buildAuxControlCommand(cmd, lightControl, lightThreshold, outputControl)) return false;

	return sendConfigCommandAsync(cmd, callback);
}



bool LD2410Async::requestAuxControlSettingsAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request Aux Control Settings");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::requestAuxControlSettingsCommandData, callback);
}


bool LD2410Async::beginAutoConfigAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Begin auto config");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::beginAutoConfigCommandData, callback);
};

bool LD2410Async::requestAutoConfigStatusAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reqtest auto config status");

	if (asyncIsBusy()) return false;

	return sendConfigCommandAsync(LD2410Defs::requestAutoConfigStatusCommandData, callback);
}
/**********************************************************************************
* High level commands that combine several native commands
***********************************************************************************/
// It is recommend to always use these commands if feasible,
// since they streamline the inconsistencies in the native requesr and config commands
// and reduce the total number of commands that have to be called.


bool LD2410Async::requestAllStaticDataAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request all static data");

	if (asyncIsBusy()) return false;


	if (!resetCommandSequence()) return false;

	if (!addCommandToSequence(LD2410Defs::requestFirmwareCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestMacAddressCommandData)) return false;

	return executeCommandSequenceAsync(callback);
}

bool LD2410Async::requestAllConfigSettingsAsync(AsyncCommandCallback callback) {
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Request all config data");

	if (asyncIsBusy()) return false;



	if (!resetCommandSequence()) return false;
	if (!addCommandToSequence(LD2410Defs::requestDistanceResolutionCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestParamsCommandData)) return false;
	if (!addCommandToSequence(LD2410Defs::requestAuxControlSettingsCommandData)) return false;

	resetConfigData();
	return executeCommandSequenceAsync(callback);

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
	configureAllConfigSettingsAsyncConfigActive = false;

	DEBUG_PRINT_MILLIS
		DEBUG_PRINT("configureAllConfigSettingsAsync complete. Result: ");
	DEBUG_PRINTLN((byte)result);

	if (cb) {
		cb(this, result);
	}

}

void LD2410Async::configureAllConfigSettingsAsyncConfigModeDisabledCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Warning: Disabling config mode after configureAllConfigSettingsAsync failed. Result: ");
		DEBUG_PRINTLN((byte)result);
		sender->configureAllConfigSettingsAsyncExecuteCallback(result);
	}
	else {
		// Config mode disabled successfully
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Config mode disabled, call the callback");

		sender->configureAllConfigSettingsAsyncExecuteCallback(sender->configureAllConfigSettingsAsyncResultToReport);
	}
}

void LD2410Async::configureAllConfigSettingsAsyncFinalize(LD2410Async::AsyncCommandResult resultToReport) {

	if (!configureAllConfigSettingsAsyncConfigInitialConfigMode) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Config mode was not enabled initially, disable it.");
		configureAllConfigSettingsAsyncResultToReport = resultToReport;

		if (!disableConfigModeInternalAsync(configureAllConfigSettingsAsyncConfigModeDisabledCallback)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Disabling config mode after configureAllConfigSettingsAsync failed.");
			configureAllConfigSettingsAsyncExecuteCallback(LD2410Async::AsyncCommandResult::FAILED);
		}
	}
	else {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Config mode was enabled initially, no need to disable it, just execute the callback.");

		configureAllConfigSettingsAsyncExecuteCallback(resultToReport);
	}
}


void LD2410Async::configureAllConfigSettingsAsyncWriteConfigCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
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
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Could not build the command sequence to save the config data");
		return false;
	}

	if (commandSequenceBufferCount == 0) {
		DEBUG_PRINT_MILLIS
			DEBUG_PRINTLN("No config changes detected, not need to write anything");
	}

	if (!executeCommandSequenceAsync(configureAllConfigSettingsAsyncWriteConfigCallback)) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Starting command sequence to write config data failed.");

		return false;
	}
	return true;

};

void LD2410Async::configureAllConfigSettingsAsyncRequestAllConfigDataCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != LD2410Async::AsyncCommandResult::SUCCESS) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Requesting current config data failed. Result: ");
		DEBUG_PRINTLN((byte)result);
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
		if (executeCommandSequenceAsync(configureAllConfigSettingsAsyncRequestAllConfigDataCallback)) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Requesting current config data");
			return true;
		}
		else {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Error: Starting command sequence to request current config data failed.");
		}
	}
	return false;
}

void LD2410Async::configureAllConfigSettingsAsyncConfigModeEnabledCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result != AsyncCommandResult::SUCCESS) {

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Enabling config mode failed. Result: ");
		DEBUG_PRINTLN((byte)result);
		sender->configureAllConfigSettingsAsyncFinalize(result);
		return;
	};

	//Got ack of enable config mode command
	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Config mode enabled.");

	bool ret = false;
	if (sender->configureAllConfigSettingsAsyncWriteFullConfig) {
		//If we save all changes anyway, no need to request current config data first
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Saving all data is forced, save directly");
		ret = sender->configureAllConfigSettingsAsyncWriteConfig();

	}
	else {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Request current config data");
		ret = sender->configureAllConfigSettingsAsyncRequestAllConfigData();
	}
	if (!ret) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error: Starting config data write or request of current config data failed.");
		sender->configureAllConfigSettingsAsyncFinalize(AsyncCommandResult::FAILED);
	}


}

bool LD2410Async::configureAllConfigSettingsAsync(const LD2410Types::ConfigData& configToWrite, bool writeAllConfigData, AsyncCommandCallback callback)
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
	configureAllConfigSettingsAsyncConfigInitialConfigMode = isConfigModeEnabled();

	if (!configureAllConfigSettingsAsyncConfigInitialConfigMode) {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Enable the config mode");
		return enableConfigModeInternalAsync(configureAllConfigSettingsAsyncConfigModeEnabledCallback);
	}
	else {
		if (configureAllConfigSettingsAsyncWriteFullConfig) {
			//If we save all changes anyway, no need to request current config data first
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Saving all data is force and configmode is enable -> save directly");
			return configureAllConfigSettingsAsyncWriteConfig();
		}
		else {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Config mode is already enabled, just request all config data");
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

void LD2410Async::rebootAsyncFinialize(LD2410Async::AsyncCommandResult result) {
	if (rebootAsyncPending) {
		rebootAsyncPending = false;
		executeCommandSequenceOnceTicker.detach();
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Reboot completed");
		executeCommandSequenceAsyncExecuteCallback(result);
	}
}

void LD2410Async::rebootAsyncRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {

		//Not necesassary, since the first data frame we receive after the reboot will set these variables back, just at the right point in time and if this does not happen, 
		// we are in trouble resp. likely stuck in config mode anyway
		//sender->configModeEnabled = false;
		//sender->engineeringModeEnabled = false;

		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Reboot initiated");
		if (sender->rebootAsyncDontWaitForNormalOperationAfterReboot || sender->rebootAsyncWaitTimeoutMs == 0) {
			DEBUG_PRINT_MILLIS;
			DEBUG_PRINTLN("Triggering the reboot callback directly since the paras of the method want us not to wait till normal opertion resumes.");
			sender->rebootAsyncFinialize(result);
		}
		else {
			if (sender->rebootAsyncPending) {
				DEBUG_PRINT_MILLIS;
				DEBUG_PRINT("Will be waiting ");
				DEBUG_PRINT(sender->rebootAsyncWaitTimeoutMs);
				DEBUG_PRINTLN(" ms for normal operation to resume.");
				sender->executeCommandSequenceOnceTicker.once_ms(sender->rebootAsyncWaitTimeoutMs, [sender]() {
					DEBUG_PRINT_MILLIS;
					DEBUG_PRINTLN("Timeout period while waiting for normaler operation to resume has elapsed.");
					sender->rebootAsyncFinialize(LD2410Async::AsyncCommandResult::TIMEOUT);
					});
			}
			else {
				DEBUG_PRINT_MILLIS;
				DEBUG_PRINTLN("It seems that reboot has been finalized already (maybe the callback for from processData has already ben triggered)");
			}
		}
	}
	else {
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINT("Error! Could not initiate reboot. Result: ");
		DEBUG_PRINTLN((int)result);
		sender->rebootAsyncFinialize(result);
	}

}

void LD2410Async::rebootAsyncEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
	if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
		//Got ack of enable config mode command
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Config mode enabled before reboot");
		sender->sendCommandAsync(LD2410Defs::rebootCommandData, rebootAsyncRebootCallback);
	}
	else {
		//Config mode command timeout or canceled
		//Just execute the callback
		DEBUG_PRINT_MILLIS;
		DEBUG_PRINTLN("Error! Could not enabled config mode before reboot");
		sender->rebootAsyncFinialize(result);
	}
}


bool LD2410Async::rebootAsync(bool dontWaitForNormalOperationAfterReboot, AsyncCommandCallback callback) {


	DEBUG_PRINT_MILLIS;
	DEBUG_PRINTLN("Reboot");

	if (asyncIsBusy()) return false;

	//"Missusing" the variables for the command sequence
	executeCommandSequencePending = true;
	executeCommandSequenceCallback = callback;
	executeCommandSequenceInitialConfigModeState = configModeEnabled;
	executeCommandSequenceStartMs = millis();
	rebootAsyncDontWaitForNormalOperationAfterReboot = dontWaitForNormalOperationAfterReboot;

	//Force config mode (just to be sure)
	rebootAsyncPending = enableConfigModeInternalAsync(true, rebootAsyncEnableConfigModeCallback);

	return rebootAsyncPending;

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
* Rest config data
***********************************************************************************/
//Resets the values of configdata to their inital value
void LD2410Async::resetConfigData() {
	configData = LD2410Types::ConfigData();
}


/**********************************************************************************
* Inactivity handling
***********************************************************************************/
void LD2410Async::handleInactivityRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
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

void LD2410Async::handleInactivityDisableConfigmodeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {


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
		unsigned long timeoutToUse = inactivityHandlingTimeoutMs;
		if (timeoutToUse < asyncCommandTimeoutMs + 1000) {
			timeoutToUse = asyncCommandTimeoutMs + 1000;
		}
		unsigned long currentTime = millis();
		unsigned long inactiveDurationMs = currentTime - lastSensorActivityTimestamp;
		if (lastSensorActivityTimestamp != 0 && inactiveDurationMs > timeoutToUse) {
			if (inactivityHandlingStep == 0 || currentTime - lastInactivityHandlingTimestamp > asyncCommandTimeoutMs + 1000) {
				lastInactivityHandlingTimestamp = currentTime;

				switch (inactivityHandlingStep++) {
				case 0:
					DEBUG_PRINT_MILLIS;
					DEBUG_PRINTLN("Inactivity handling cancels pending async operations");
					asyncCancel();
					break;
				case 1:
					DEBUG_PRINT_MILLIS;
					DEBUG_PRINTLN("Inactivity handling tries to force diable config mode");
					//We dont care about the result resp. a callback, if the command has the desired effect, fine, otherwise will try to reboot the sensor anyway
					disableConfigModeInternalAsync(true, nullptr);
					break;
				case 3:
					DEBUG_PRINT_MILLIS;
					DEBUG_PRINTLN("Inactivity handling reboots the sensor");
					//We dont care about the result, if the command has the desired effect, fine, otherwise will try to reboot the sensor anyway
					rebootAsync(true, nullptr);

					break;
				default:
					DEBUG_PRINT_MILLIS;
					DEBUG_PRINTLN("Inactivity handling could not revert sensor to normal operation. Reset the inactivity timeout and try again after the configured inactivity period.");
					//Inactivity handling has tried everything it can do, so reset the inactivity handling steps and call heartbeat. This will ensure inactivity handling will get called again after the inactivity handling period
					inactivityHandlingStep = 0;
					heartbeat();
					break;
				}
			}
		}
		else {
			inactivityHandlingStep = 0;
		}
	}
}

void LD2410Async::heartbeat() {
	lastSensorActivityTimestamp = millis();
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
		sendCommandAsyncHandleTimeout();
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
