#pragma once


#define ENABLE_DEBUG

//#define ENABLE_DEBUG_DATA

#ifndef ARDUINO_ARCH_ESP32
#error "The LD2410Async library is only supported on ESP32 platforms."
#endif


#ifdef ENABLE_DEBUG

#define DEBUG_PRINT_MILLIS              \
    {                                   \
        Serial.print(millis());         \
		Serial.print(" ");    \
	}


#define DEBUG_PRINT(...)               \
    {                                       \
        Serial.print(__VA_ARGS__);         \
    }


#define DEBUG_PRINTLN(...)             \
    {                                       \
        Serial.println(__VA_ARGS__);         \
    }

#define DEBUG_PRINTBUF(...)             \
    {                                       \
        printBuf(__VA_ARGS__);         \
    }

#else

#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINTBUF(...)
#endif

#ifdef ENABLE_DEBUG_DATA

#define DEBUG_PRINT_DATA(...)               \
    {                                       \
        Serial.print(__VA_ARGS__);         \
    }


#define DEBUG_PRINTLN_DATA(...)             \
    {                                       \
        Serial.println(__VA_ARGS__);         \
    }

#define DEBUG_PRINTBUF_DATA(...)             \
    {                                       \
        printBuf(__VA_ARGS__);         \
    }
#else
#define DEBUG_PRINT_DATA(...)
#define DEBUG_PRINTLN_DATA(...)
#define DEBUG_PRINTBUF_DATA(...)
#endif

#include "Arduino.h"
#define LD2410_BUFFER_SIZE 0x40



/**
 * @brief Asynchronous driver for the LD2410 human presence radar sensor.
 *
 * The LD2410 is a mmWave radar sensor capable of detecting both moving and
 * stationary targets, reporting presence, distance, and per-gate signal strength.
 * This class implements a non-blocking, asynchronous interface for communicating
 * with the sensor over a UART stream (HardwareSerial, SoftwareSerial, etc.).
 *
 * ## Features
 * - Continuous background task that parses incoming frames and updates data.
 * - Access to latest detection results via getDetectionData() or getDetectionDataRef().
 * - Access to current configuration via getConfigData() or getConfigDataRef().
 * - Asynchronous commands for configuration (with callbacks).
 * - Support for engineering mode (per-gate signal values).
 * - Automatic inactivity handling (optional recovery and reboot).
 * - Utility methods for safe enum conversion and debugging output.
 *
 * ## Accessing data
 * You can either clone the structs (safe to modify) or access them by reference (efficient read-only):
 *
 * ### Example: Access detection data without cloning
 * @code
 *   const DetectionData& data = radar.getDetectionDataRef();  // no copy
 *   Serial.print("Target state: ");
 *   Serial.println(static_cast<int>(data.targetState));
 * @endcode
 *
 * ### Example: Clone config data, modify, and write back
 * @code
 *   ConfigData cfg = radar.getConfigData();  // clone
 *   cfg.noOneTimeout = 60;
 *   radar.setConfigDataAsync(cfg, [](LD2410Async* sender,
 *                                    AsyncCommandResult result,
 *                                    byte) {
 *     if (result == AsyncCommandResult::SUCCESS) {
 *       Serial.println("Config updated successfully!");
 *     }
 *   });
 * @endcode
 *
 * ## Usage
 * Typical workflow:
 * 1. Construct with a reference to a Stream object connected to the sensor.
 * 2. Call begin() to start the background task.
 * 3. Register callbacks for detection data and/or config updates.
 * 4. Use async commands to adjust sensor configuration as needed.
 * 5. Call end() to stop background processing if no longer required.
 *
 * Example:
 * @code
 *   HardwareSerial radarSerial(2);
 *   LD2410Async radar(radarSerial);
 *
 *   void setup() {
 *     Serial.begin(115200);
 *     radar.begin();
 *
 *     // Register callback for detection updates
 *     radar.registerDetectionDataReceivedCallback([](LD2410Async* sender, bool presenceDetetced, byte userData) {
 *       sender->getDetectionDataRef().print();  // direct access, no copy
 *     });
 *   }
 *
 *   void loop() {
 *     // Other application logic
 *   }
 * @endcode
 */

class LD2410Async {
public:
	/**
	 * @brief Represents the current target detection state reported by the radar.
	 *
	 * Values can be combined internally by the sensor depending on its
	 * signal evaluation logic. The AUTO-CONFIG states are special values
	 * that are only reported while the sensor is running its
	 * self-calibration routine.
	 */

	enum class TargetState {
		NO_TARGET = 0,                  ///< No moving or stationary target detected.
		MOVING_TARGET = 1,              ///< A moving target has been detected.
		STATIONARY_TARGET = 2,          ///< A stationary target has been detected.
		MOVING_AND_STATIONARY_TARGET = 3, ///< Both moving and stationary targets detected.
		AUTOCONFIG_IN_PROGRESS = 4,     ///< Auto-configuration routine is running.
		AUTOCONFIG_SUCCESS = 5,         ///< Auto-configuration completed successfully.
		AUTOCONFIG_FAILED = 6           ///< Auto-configuration failed.
	};

	/**
	 * @brief Safely converts a numeric value to a TargetState enum.
	 *
	 * @param value Raw numeric value (0–6 expected).
	 *              - 0 → NO_TARGET
	 *              - 1 → MOVING_TARGET
	 *              - 2 → STATIONARY_TARGET
	 *              - 3 → MOVING_AND_STATIONARY_TARGET
	 *              - 4 → AUTOCONFIG_IN_PROGRESS
	 *              - 5 → AUTOCONFIG_SUCCESS
	 *              - 6 → AUTOCONFIG_FAILED
	 * @returns The matching TargetState value, or NO_TARGET if invalid.
	 */
	static TargetState toTargetState(int value) {
		switch (value) {
		case 0: return TargetState::NO_TARGET;
		case 1: return TargetState::MOVING_TARGET;
		case 2: return TargetState::STATIONARY_TARGET;
		case 3: return TargetState::MOVING_AND_STATIONARY_TARGET;
		case 4: return TargetState::AUTOCONFIG_IN_PROGRESS;
		case 5: return TargetState::AUTOCONFIG_SUCCESS;
		case 6: return TargetState::AUTOCONFIG_FAILED;
		default: return TargetState::NO_TARGET; // safe fallback
		}
	}

	/**
	 * @brief Converts a TargetState enum value to a human-readable String.
	 *
	 * Useful for printing detection results in logs or Serial Monitor.
	 *
	 * @param state TargetState enum value.
	 * @returns Human-readable string such as "No target", "Moving target", etc.
	 */
	static String targetStateToString(TargetState state) {
		switch (state) {
		case TargetState::NO_TARGET: return "No target";
		case TargetState::MOVING_TARGET: return "Moving target";
		case TargetState::STATIONARY_TARGET: return "Stationary target";
		case TargetState::MOVING_AND_STATIONARY_TARGET: return "Moving and stationary target";
		case TargetState::AUTOCONFIG_IN_PROGRESS: return "Auto-config in progress";
		case TargetState::AUTOCONFIG_SUCCESS: return "Auto-config success";
		case TargetState::AUTOCONFIG_FAILED: return "Auto-config failed";
		default: return "Unknown";
		}
	}



	/**
	 * @brief Result of an asynchronous command execution.
	 *
	 * Every async command reports back its outcome via the callback.
	 */
	enum class AsyncCommandResult : byte {
		SUCCESS,    ///< Command completed successfully and ACK was received.
		FAILED,     ///< Command failed (sensor responded with negative ACK).
		TIMEOUT,    ///< No ACK received within the expected time window.
		CANCELED    ///< Command was canceled by the user before completion.
	};



	/**
	 * @brief Light-dependent control status of the auxiliary output.
	 *
	 * The radar sensor can control an external output based on ambient
	 * light level in combination with presence detection.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 */
	enum class LightControl {
		NOT_SET = -1,          ///< Placeholder resp. inital value. Do not use as a config value (will result in abortion of the config command).
		NO_LIGHT_CONTROL = 0,  ///< Output is not influenced by light levels.
		LIGHT_BELOW_THRESHOLD = 1, ///< Condition: light < threshold.
		LIGHT_ABOVE_THRESHOLD = 2  ///< Condition: light ≥ threshold.
	};
	/**
	 * @brief Safely converts a numeric value to a LightControl enum.
	 *
	 * @param value Raw numeric value (0–2 expected).
	 *              - 0 → NO_LIGHT_CONTROL
	 *              - 1 → LIGHT_BELOW_THRESHOLD
	 *              - 2 → LIGHT_ABOVE_THRESHOLD
	 * @returns The matching LightControl value, or NOT_SET if invalid.
	 */
	static LightControl toLightControl(int value) {
		switch (value) {
		case 0: return LightControl::NO_LIGHT_CONTROL;
		case 1: return LightControl::LIGHT_BELOW_THRESHOLD;
		case 2: return LightControl::LIGHT_ABOVE_THRESHOLD;
		default: return LightControl::NOT_SET;
		}
	}


	/**
	 * @brief Logic level behavior of the auxiliary output pin.
	 *
	 * Determines whether the output pin is active-high or active-low
	 * in relation to presence detection.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 */
	enum class OutputControl {
		NOT_SET = -1,                ///< Placeholder resp. inital value. Do not use as a config value (will result in abortion of the config command).
		DEFAULT_LOW_DETECTED_HIGH = 0, ///< Default low, goes HIGH when detection occurs.
		DEFAULT_HIGH_DETECTED_LOW = 1  ///< Default high, goes LOW when detection occurs.
	};

	/**
	 * @brief Safely converts a numeric value to an OutputControl enum.
	 *
	 * @param value Raw numeric value (0–1 expected).
	 *              - 0 → DEFAULT_LOW_DETECTED_HIGH
	 *              - 1 → DEFAULT_HIGH_DETECTED_LOW
	 * @returns The matching OutputControl value, or NOT_SET if invalid.
	 */
	static OutputControl toOutputControl(int value) {
		switch (value) {
		case 0: return OutputControl::DEFAULT_LOW_DETECTED_HIGH;
		case 1: return OutputControl::DEFAULT_HIGH_DETECTED_LOW;
		default: return OutputControl::NOT_SET;
		}
	}

	/**
	 * @brief State of the automatic threshold configuration routine.
	 *
	 * Auto-config adjusts the sensitivity thresholds for optimal detection
	 * in the current environment. This process may take several seconds.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 */
	enum class AutoConfigStatus {
		NOT_SET = -1,    ///< Status not yet retrieved.
		NOT_IN_PROGRESS, ///< Auto-configuration not running.
		IN_PROGRESS,     ///< Auto-configuration is currently running.
		COMPLETED        ///< Auto-configuration finished (success or failure).
	};

	/**
	 * @brief Safely converts a numeric value to an AutoConfigStatus enum.
	 *
	 * @param value Raw numeric value (0–2 expected).
	 *              - 0 → NOT_IN_PROGRESS
	 *              - 1 → IN_PROGRESS
	 *              - 2 → COMPLETED
	 * @returns The matching AutoConfigStatus value, or NOT_SET if invalid.
	 */
	static AutoConfigStatus toAutoConfigStatus(int value) {
		switch (value) {
		case 0: return AutoConfigStatus::NOT_IN_PROGRESS;
		case 1: return AutoConfigStatus::IN_PROGRESS;
		case 2: return AutoConfigStatus::COMPLETED;
		default: return AutoConfigStatus::NOT_SET;
		}
	}


	/**
	 * @brief Supported baud rates for the sensor’s UART interface.
	 *
	 * These values correspond to the configuration commands accepted
	 * by the LD2410. After changing the baud rate, a sensor reboot
	 * is required, and the ESP32’s UART must be reconfigured to match.
	 */
	enum class Baudrate {
		BAUDRATE_9600 = 1,  ///< 9600 baud.
		BAUDRATE_19200 = 2,  ///< 19200 baud.
		BAUDRATE_38400 = 3,  ///< 38400 baud.
		BAUDRATE_57600 = 4,  ///< 57600 baud.
		BAUDRATE_115200 = 5,  ///< 115200 baud.
		BAUDRATE_230500 = 6,  ///< 230400 baud.
		BAUDRATE_256000 = 7,  ///< 256000 baud (factory default).
		BAUDRATE_460800 = 8   ///< 460800 baud (high-speed).
	};

	/**
	 * @brief Distance resolution per gate for detection.
	 *
	 * The resolution defines how fine-grained the distance measurement is:
	 *   - RESOLUTION_75CM: Coarser, maximum range up to ~6 m, each gate ≈ 0.75 m wide.
	 *   - RESOLUTION_20CM: Finer, maximum range up to ~1.8 m, each gate ≈ 0.20 m wide.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 */
	enum class DistanceResolution {
		NOT_SET = -1,          ///< Placeholder resp. inital value. Do not use as a config value (will result in abortion of the config command).
		RESOLUTION_75CM = 0,   ///< Each gate ~0.75 m, max range ~6 m.
		RESOLUTION_20CM = 1    ///< Each gate ~0.20 m, max range ~1.8 m.
	};
	/**
	 * @brief Safely converts a numeric value to a DistanceResolution enum.
	 *
	 * @param value Raw numeric value (typically from a sensor response).
	 *              Expected values:
	 *                - 0 → RESOLUTION_75CM
	 *                - 1 → RESOLUTION_20CM
	 * @returns The matching DistanceResolution value, or NOT_SET if invalid.
	 */
	static DistanceResolution toDistanceResolution(int value) {
		switch (value) {
		case 0: return DistanceResolution::RESOLUTION_75CM;
		case 1: return DistanceResolution::RESOLUTION_20CM;
		default: return DistanceResolution::NOT_SET;
		}
	}

	/**
	  * @brief Holds the most recent detection data reported by the radar.
	  *
	  * This structure is continuously updated as new frames arrive.
	  * Values reflect either the basic presence information or, if
	  * engineering mode is enabled, per-gate signal details.
	  */
	struct DetectionData
	{
		unsigned long timestamp = 0; ///< Timestamp (ms since boot) when this data was received.

		// === Basic detection results ===

		bool engineeringMode = false; ///< True if engineering mode data was received.

		bool presenceDetected = false;           ///< True if any target is detected.
		bool movingPresenceDetected = false;     ///< True if a moving target is detected.
		bool stationaryPresenceDetected = false; ///< True if a stationary target is detected.

		TargetState targetState = TargetState::NO_TARGET; ///< Current detection state.
		unsigned int movingTargetDistance = 0; ///< Distance (cm) to the nearest moving target.
		byte movingTargetSignal = 0; ///< Signal strength (0–100) of the moving target.
		unsigned int stationaryTargetDistance = 0; ///< Distance (cm) to the nearest stationary target.
		byte stationaryTargetSignal = 0; ///< Signal strength (0–100) of the stationary target.
		unsigned int detectedDistance = 0; ///< General detection distance (cm).

		// === Engineering mode data ===
		byte movingTargetGateSignalCount = 0; ///< Number of gates with moving target signals.
		byte movingTargetGateSignals[9] = { 0 }; ///< Per-gate signal strengths for moving targets.

		byte stationaryTargetGateSignalCount = 0; ///< Number of gates with stationary target signals.
		byte stationaryTargetGateSignals[9] = { 0 }; ///< Per-gate signal strengths for stationary targets.

		byte lightLevel = 0; ///< Reported ambient light level (0–255).
		bool outPinStatus = 0; ///< Current status of the OUT pin (true = high, false = low).


		/**
		 * @brief Debug helper: print detection data contents to Serial.
		 */
		void print() const {
			Serial.println("=== DetectionData ===");

			Serial.print("  Timestamp: ");
			Serial.println(timestamp);

			Serial.print("  Engineering Mode: ");
			Serial.println(engineeringMode ? "Yes" : "No");

			Serial.print("  Target State: ");
			Serial.println(static_cast<int>(targetState));

			Serial.print("  Moving Target Distance (cm): ");
			Serial.println(movingTargetDistance);

			Serial.print("  Moving Target Signal: ");
			Serial.println(movingTargetSignal);

			Serial.print("  Stationary Target Distance (cm): ");
			Serial.println(stationaryTargetDistance);

			Serial.print("  Stationary Target Signal: ");
			Serial.println(stationaryTargetSignal);

			Serial.print("  Detected Distance (cm): ");
			Serial.println(detectedDistance);

			Serial.print("  Light Level: ");
			Serial.println(lightLevel);

			Serial.print("  OUT Pin Status: ");
			Serial.println(outPinStatus ? "High" : "Low");

			// --- Engineering mode fields ---
			if (engineeringMode) {
				Serial.println("  --- Engineering Mode Data ---");

				Serial.print("  Moving Target Gate Signal Count: ");
				Serial.println(movingTargetGateSignalCount);

				Serial.print("  Moving Target Gate Signals: ");
				for (int i = 0; i < movingTargetGateSignalCount; i++) {
					Serial.print(movingTargetGateSignals[i]);
					if (i < movingTargetGateSignalCount - 1) Serial.print(",");
				}
				Serial.println();

				Serial.print("  Stationary Target Gate Signal Count: ");
				Serial.println(stationaryTargetGateSignalCount);

				Serial.print("  Stationary Target Gate Signals: ");
				for (int i = 0; i < stationaryTargetGateSignalCount; i++) {
					Serial.print(stationaryTargetGateSignals[i]);
					if (i < stationaryTargetGateSignalCount - 1) Serial.print(",");
				}
				Serial.println();
			}

			Serial.println("======================");
		}

	};

	/**
	 * @brief Stores the sensor’s configuration parameters.
	 *
	 * This structure represents both static capabilities
	 * (e.g. number of gates) and configurable settings
	 * (e.g. sensitivities, timeouts, resolution).
	 *
	 * The values are typically filled by request commands
	 * such as requestAllConfigData() or requestGateParametersAsync() or
	 * requestAuxControlSettingsAsync() or requestDistanceResolutioncmAsync().
	 */
	struct ConfigData {
		// === Radar capabilities ===
		byte numberOfGates = 0; ///< Number of distance gates (2–8). This member is readonly resp. changing its value will not influence the radar sentting when setConfigDataAsync() is called.

		byte maxMotionDistanceGate = 0; ///< Furthest gate used for motion detection.
		byte maxStationaryDistanceGate = 0; ///< Furthest gate used for stationary detection.

		// === Per-gate sensitivity settings ===
		byte distanceGateMotionSensitivity[9] = { 0 }; ///< Motion sensitivity values per gate (0–100). 
		byte distanceGateStationarySensitivity[9] = { 0 }; ///< Stationary sensitivity values per gate (0–100).

		// === General detection parameters ===
		unsigned short noOneTimeout = 0; ///< Timeout (seconds) until "no presence" is declared.

		// === Resolution and auxiliary controls ===
		DistanceResolution distanceResolution = DistanceResolution::NOT_SET; ///< Current distance resolution. A reboot is required to activate changed setting after calling setConfigDataAsync() is called.
		byte lightThreshold = 0; ///< Threshold for auxiliary light control (0–255).
		LightControl lightControl = LightControl::NOT_SET; ///< Light-dependent auxiliary control mode.
		OutputControl outputControl = OutputControl::NOT_SET; ///< Logic configuration of the OUT pin.

		/**
		 * @brief Debug helper: print configuration contents to Serial.
		 */
		void print() const {
			Serial.println("=== ConfigData ===");

			Serial.print("  Number of Gates: ");
			Serial.println(numberOfGates);

			Serial.print("  Max Motion Distance Gate: ");
			Serial.println(maxMotionDistanceGate);

			Serial.print("  Max Stationary Distance Gate: ");
			Serial.println(maxStationaryDistanceGate);

			Serial.print("  Motion Sensitivity: ");
			for (int i = 0; i < 9; i++) {
				Serial.print(distanceGateMotionSensitivity[i]);
				if (i < 8) Serial.print(",");
			}
			Serial.println();

			Serial.print("  Stationary Sensitivity: ");
			for (int i = 0; i < 9; i++) {
				Serial.print(distanceGateStationarySensitivity[i]);
				if (i < 8) Serial.print(",");
			}
			Serial.println();

			Serial.print("  No One Timeout: ");
			Serial.println(noOneTimeout);

			Serial.print("  Distance Resolution: ");
			Serial.println(static_cast<int>(distanceResolution));

			Serial.print("  Light Threshold: ");
			Serial.println(lightThreshold);

			Serial.print("  Light Control: ");
			Serial.println(static_cast<int>(lightControl));

			Serial.print("  Output Control: ");
			Serial.println(static_cast<int>(outputControl));

			Serial.println("===================");
		}
	};



	/**
	 * @brief Callback signature for asynchronous command completion.
	 *
	 * @param sender   Pointer to the LD2410Async instance that triggered the callback.
	 * @param result   Outcome of the async operation (SUCCESS, FAILED, TIMEOUT, CANCELED).
	 * @param userData User-specified value passed when registering the callback.
	 */
	typedef void (*AsyncCommandCallback)(LD2410Async* sender, AsyncCommandResult result, byte userData);

	/**
	 * @brief Generic callback signature used for simple notifications.
	 *
	 * @param sender   Pointer to the LD2410Async instance that triggered the callback.
	 * @param userData User-specified value passed when registering the callback.
	 */
	typedef void (*GenericCallback)(LD2410Async* sender, byte userData);

	/**
	 * @brief Callback type for receiving detection data events.
	 *
	 * This callback is invoked whenever new detection data is processed.
	 * It provides direct access to the LD2410Async instance, along with
	 * a quick flag for presence detection so that applications which only
	 * care about presence can avoid parsing the full DetectionData struct.
	 *
	 * @param sender            Pointer to the LD2410Async instance that triggered the callback.
	 * @param presenceDetected  True if the radar currently detects presence (moving or stationary), false otherwise.
	 * @param userData          User-defined value passed when registering the callback.
	 */
	typedef void(*DetectionDataCallback)(LD2410Async* sender, bool presenceDetected, byte userData);


private:
	//Pointer to the Serial of the LD2410
	Stream* sensor;

	//Read frame enums, members and methods
	enum ReadFrameState {
		WAITING_FOR_HEADER,
		ACK_HEADER,
		DATA_HEADER,
		READ_ACK_SIZE,
		READ_DATA_SIZE,
		READ_ACK_PAYLOAD,
		READ_DATA_PAYLOAD
	};
	enum FrameReadResponse
	{
		FAIL = 0,
		ACK,
		DATA
	};
	int readFrameHeaderIndex = 0;
	int payloadSize = 0;
	ReadFrameState readFrameState = ReadFrameState::WAITING_FOR_HEADER;

	bool readFramePayloadSize(byte b, ReadFrameState nextReadFrameState);
	FrameReadResponse readFramePayload(byte b, const byte* tailPattern, LD2410Async::FrameReadResponse succesResponseType);
	FrameReadResponse readFrame();


	//Buffer for received data
	byte receiveBuffer[LD2410_BUFFER_SIZE];
	byte receiveBufferIndex = 0;

	//Vars for async command sequences
	static constexpr size_t MAX_COMMAND_SEQUENCE = 15;
	byte commandSequenceBuffer[MAX_COMMAND_SEQUENCE][LD2410_BUFFER_SIZE];
	byte commandSequenceBufferCount = 0;

	unsigned long sendAsyncSequenceStartMs = 0;
	AsyncCommandCallback sendAsyncSequenceCallback = nullptr;
	byte sendAsyncSequenceUserData = 0;
	int sendAsyncSequenceIndex = 0;
	bool sendAsyncSequenceInitialConfigModeState = false;

	void executeAsyncSequenceCallback(AsyncCommandResult result);
	static void sendCommandSequenceAsyncDisableConfigModeCallback(LD2410Async* sender, AsyncCommandResult result, byte userData = 0);
	static void sendCommandSequenceAsyncCommandCallback(LD2410Async* sender, AsyncCommandResult result, byte userData = 0);
	static void sendCommandSequenceAsyncEnableConfigModeCallback(LD2410Async* sender, AsyncCommandResult result, byte userData = 0);
	bool sendCommandSequenceAsync(AsyncCommandCallback callback, byte userData = 0);
	bool addCommandToSequence(const byte* command);
	bool resetCommandSequence();


	//Inactivity handling

	void heartbeat();

	bool inactivityHandlingEnabled = true;
	unsigned long lastActivityMs = 0;
	bool handleInactivityExitConfigModeDone = false;
	void handleInactivity();
	static void handleInactivityRebootCallback(LD2410Async* sender, AsyncCommandResult result, byte userData);
	static void handleInactivityDisableConfigmodeCallback(LD2410Async* sender, AsyncCommandResult result, byte userData);
	const unsigned long activityTimeoutMs = 60000;


	//Reboot
	static void rebootEnableConfigModeCallback(LD2410Async* sender, AsyncCommandResult result, byte userData = 0);
	static void rebootRebootCallback(LD2410Async* sender, AsyncCommandResult result, byte userData = 0);






	bool processAck();
	bool processData();


	GenericCallback configUpdateReceivedReceivedCallback = nullptr;
	byte configUpdateReceivedReceivedCallbackUserData = 0;
	void executeConfigUpdateReceivedCallback();

	GenericCallback configChangedCallback = nullptr;
	byte configChangedCallbackUserData = 0;
	void executeConfigChangedCallback();



	DetectionDataCallback detectionDataCallback = nullptr;
	byte detectionDataCallbackUserData = 0;

	void sendCommand(const byte* command);

	TaskHandle_t taskHandle = NULL;
	bool taskStop = false;
	void taskLoop();


	//Private async variables and methods
	const unsigned long asyncCommandTimeoutMs = 5000;

	bool sendCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData = 0);
	void executeAsyncCommandCallback(byte commandCode, AsyncCommandResult result);
	void handleAsyncCommandCallbackTimeout();

	bool sendConfigCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData = 0);



	AsyncCommandCallback asyncCommandCallback = nullptr;
	byte asyncCommandCallbackUserData = 0;
	unsigned long asyncCommandStartMs = 0;
	byte asyncCommandCommandCode = 0;


	void processReceivedData();



public:
	/**
	   * @brief Latest detection results from the radar.
	   *
	   * Updated automatically whenever new data frames are received.
	   * Use registerDetectionDataReceivedCallback() to be notified
	   * whenever this struct changes.
	   */
	DetectionData detectionData;

	/**
	 * @brief Current configuration parameters of the radar.
	 *
	 * Filled when configuration query commands are issued
	 * (e.g. requestAllConfigData() or requestGateParametersAsync() ect). Can be modified and
	 * sent back using setConfigDataAsync().
	 *
	 * Structure will contain only uninitilaized data if config data is not queried explicitly.
	 */
	ConfigData configData;

	/**
	* @brief Protocol version reported by the radar.
	*
	* This value is set when entering config mode. It can be useful
	* for compatibility checks between firmware and library.
	*/
	unsigned long protocolVersion = 0;

	/**
	* @brief Buffer size reported by the radar protocol.
	*
	* Set when entering config mode. Typically not required by users
	* unless debugging low-level protocol behavior.
	*/
	unsigned long bufferSize = 0;

	/**
	 * @brief True if the sensor is currently in config mode.
	 *
	 * Config mode must be enabled using enableConfigModeAsync() before sending configuration commands.
	 * After sending config commands, always disable the config mode using disableConfigModeAsync(),
	 * otherwiese the radar will not send any detection data.
	 */
	bool configModeEnabled = false;


	/**
	 * @brief True if the sensor is currently in engineering mode.
	 *
	 * In engineering mode, the radar sends detailed per-gate
	 * signal data in addition to basic detection data.
	 *
	 * Use enableEngineeringModeAsync() and disableEngineeringModeAsync() to control the engineering mode.
	 */
	bool engineeringModeEnabled = false;

	/**
	 * @brief Firmware version string of the radar.
	 *
	 * Populated by requestFirmwareAsync(). Format is usually
	 * "major.minor.build".
	 */
	String firmware = "";

	/**
	 * @brief MAC address of the radar’s Bluetooth module (if available).
	 *
	 * Populated by requestBluetoothMacAddressAsync().
	 */
	byte mac[6];
	/**
	 * @brief MAC address as a human-readable string (e.g. "AA:BB:CC:DD:EE:FF").
	 *
	 * Populated by requestBluetoothMacAddressAsync().
	 */
	String macString = "";


	/**
	* @brief Current status of the auto-configuration routine.
	*
	* Updated by requestAutoConfigStatusAsync().
	*/
	AutoConfigStatus	autoConfigStatus = AutoConfigStatus::NOT_SET;



	/**********************************************************************************
	* Constrcutor
	***********************************************************************************/

	/**
	* @brief Constructs a new LD2410Async instance bound to a given serial stream.
	*
	* The sensor communicates over a UART interface. Pass the corresponding
	* Stream object (e.g. HardwareSerial, SoftwareSerial, or another compatible
	* implementation) that is connected to the LD2410 sensor.
	*
	* Example:
	* @code
	*   HardwareSerial radarSerial(2);
	*   LD2410Async radar(radarSerial);
	* @endcode
	*
	* @param serial Reference to a Stream object used to exchange data with the sensor.
	*/
	LD2410Async(Stream& serial);

	/**********************************************************************************
	* begin, end
	***********************************************************************************/
	/**
	 * @brief Starts the background task that continuously reads data from the sensor.
	 *
	 * This method creates a FreeRTOS task which parses all incoming frames
	 * and dispatches registered callbacks. Without calling begin(), the
	 * sensor cannot deliver detection results asynchronously.
	 *
	 * @returns true if the task was successfully started, false if already running.
	 */
	bool begin();

	/**
	* @brief Stops the background task started by begin().
	*
	* After calling end(), no more data will be processed until begin() is called again.
	* This is useful to temporarily suspend radar processing without rebooting.
	*
	* @returns true if the task was stopped, false if it was not active.
	*/
	bool end();

	/**********************************************************************************
	* Inactivity handling
	***********************************************************************************/
	/**
	 * @brief Enables or disables automatic inactivity handling of the sensor.
	 *
	 * When inactivity handling is enabled, the library continuously monitors the time
	 * since the last activity (received data or command ACK). If no activity is detected
	 * for a longer period (defined by activityTimeoutMs), the library will attempt to
	 * recover the sensor automatically:
	 *   1. It first tries to exit config mode (even if configModeEnabled is false).
	 *   2. If no activity is restored within 5 seconds after leaving config mode,
	 *      the library reboots the sensor.
	 *
	 * This helps recover the sensor from rare cases where it gets "stuck"
	 * in config mode or stops sending data.
	 *
	 * @param enable Pass true to enable inactivity handling, false to disable it.
	 */
	void setInactivityHandling(bool enable);

	/**
	 * @brief Convenience method: enables inactivity handling.
	 *
	 * Equivalent to calling setInactivityHandling(true).
	 */
	void enableInactivityHandling() { setInactivityHandling(true); };

	/**
	 * @brief Convenience method: disables inactivity handling.
	 *
	 * Equivalent to calling setInactivityHandling(false).
	 */
	void disableInactivityHandling() { setInactivityHandling(false); };


	/**********************************************************************************
	* Data received methods
	***********************************************************************************/

	/**
	  * @brief Registers a callback for new detection data.
	  *
	  * The callback is invoked whenever a valid data frame is received
	  * from the radar, after detectionData has been updated.
	  *
	  * @param callback Function pointer with signature
	  *        void methodName(LD2410Async* sender, bool presenceDetected, byte userData).
	  * @param userData Optional value that will be passed to the callback.
	  */
	void registerDetectionDataReceivedCallback(DetectionDataCallback callback, byte userData);

	/**
	 * @brief Registers a callback for configuration changes.
	 *
	 * The callback is invoked whenever the sensor’s configuration
	 * has been successfully updated (e.g. after setting sensitivity).
	 *
	 * @param callback Function pointer with signature
	 *        void methodName(LD2410Async* sender, byte userData).
	 * @param userData Optional value that will be passed to the callback.
	 */
	void registerConfigChangedCallback(GenericCallback callback, byte userData = 0);

	/**
	 * @brief Registers a callback for configuration data updates.
	 *
	 * The callback is invoked whenever new configuration information
	 * has been received from the sensor (e.g. after requestGateParametersAsync()).
	 *
	 * @param callback Function pointer with signature
	 *        void methodName(LD2410Async* sender, byte userData).
	 * @param userData Optional value that will be passed to the callback.
	 */
	void registerConfigUpdateReceivedCallback(GenericCallback callback, byte userData = 0);

	/**********************************************************************************
	* Special async commands
	***********************************************************************************/
	/**
	 * @brief Returns a clone of the latest detection data from the radar.
	 *
	 * The returned struct contains the most recently received frame,
	 * including target state, distances, signal strengths, and
	 * (if enabled) engineering mode per-gate data.
	 *
	 * Equivalent to directly accessing the public member detectionData,
	 * but provided for encapsulation and future-proofing.
	 *
	 * @note This function will not query the sensor for data. It just returns
	 *       the data that has already been received from the sensor.
	 *
	 * ## Example: Access values from a clone
	 * @code
	 *   DetectionData data = radar.getDetectionData();  // makes a copy
	 *   if (data.targetState == TargetState::MOVING_TARGET) {
	 *     Serial.print("Moving target at distance: ");
	 *     Serial.println(data.movingTargetDistance);
	 *   }
	 * @endcode
	 *
	 * ## Do:
	 * - Use when you want a snapshot of the latest detection data.
	 * - Modify the returned struct freely without affecting the internal state.
	 *
	 * ## Don’t:
	 * - Expect this to fetch new data from the sensor (it only returns what was already received).
	 *
	 * @returns A copy of the current DetectionData.
	 */
	DetectionData getDetectionData() const;



	/**
	 * @brief Returns a clone of the current configuration data of the radar.
	 *
	 * The returned struct contains the most recently requested
	 * or received configuration values, such as sensitivities,
	 * resolution, timeouts, and auxiliary settings.
	 *
	 * Equivalent to directly accessing the public member configData,
	 * but provided for encapsulation and future-proofing.
	 *
	 * @note This function will not query the sensor for data. It just returns
	 *       the data that has already been received from the sensor.
	 *
	 * ## Example: Clone, modify, and write back
	 * @code
	 *   // Clone current config
	 *   ConfigData cfg = radar.getConfigData();
	 *
	 *   // Modify locally
	 *   cfg.noOneTimeout = 60;  // change timeout
	 *   cfg.distanceGateMotionSensitivity[3] = 80;  // adjust sensitivity
	 *
	 *   // Send modified config back to sensor
	 *   radar.setConfigDataAsync(cfg, [](LD2410Async* sender,
	 *                                    AsyncCommandResult result,
	 *                                    byte) {
	 *     if (result == AsyncCommandResult::SUCCESS) {
	 *       Serial.println("Config updated successfully!");
	 *     }
	 *   });
	 * @endcode
	 *
	 * ## Do:
	 * - Use when you want a clone of the current config to adjust and send back.
	 * - Safely modify the struct without risking internal state corruption.
	 *
	 * ## Don’t:
	 * - Assume this always reflects the live sensor config (it’s only as fresh as the last received config data).
	 *
	 * @returns A copy of the current ConfigData.
	 */
	ConfigData getConfigData() const;



	/**
	 * @brief Access the current detection data without making a copy.
	 *
	 * This returns a const reference to the internal struct. It is efficient,
	 * but the data must not be modified directly. Use this if you only want
	 * to read values.
	 *
	 * @note Since this returns a reference to the internal data, the values
	 *       may change as new frames arrive. Do not store the reference for
	 *       long-term use.
	 * @note This function will not query the sensor for data. It just returns
	 *       the data that has already been received from the sensor.
	 *
	 * ## Example: Efficient read access without cloning
	 * @code
	 *   const DetectionData& data = radar.getDetectionDataRef();  // no copy
	 *   Serial.print("Stationary signal: ");
	 *   Serial.println(data.stationaryTargetSignal);
	 * @endcode
	 *
	 * ## Do:
	 * - Use when you only need to read values quickly and efficiently.
	 * - Use when printing or inspecting live data without keeping it.
	 *
	 * ## Don’t:
	 * - Try to modify the returned struct (it’s const).
	 * - Store the reference long-term (it may be updated at any time).
	 *
	 * @returns Const reference to the current DetectionData.
	 */
	const DetectionData& getDetectionDataRef() const { return detectionData; }



	/**
	 * @brief Access the current config data without making a copy.
	 *
	 * This returns a const reference to the internal struct. It is efficient,
	 * but the data must not be modified directly. Use this if you only want
	 * to read values.
	 *
	 * @note Since this returns a reference to the internal data, the values
	 *       may change when new configuration is received. Do not store the
	 *       reference for long-term use.
	 * @note This function will not query the sensor for data. It just returns
	 *       the data that has already been received from the sensor.
	 *
	 * ## Example: Efficient read access without cloning
	 * @code
	 *   const ConfigData& cfg = radar.getConfigDataRef();  // no copy
	 *   Serial.print("Resolution: ");
	 *   Serial.println(static_cast<int>(cfg.distanceResolution));
	 * @endcode
	 *
	 * ## Do:
	 * - Use when you only want to inspect configuration quickly.
	 * - Use for efficient read-only access.
	 *
	 * ## Don’t:
	 * - Try to modify the returned struct (it’s const).
	 * - Keep the reference and assume it will remain valid forever.
	 *
	 * @returns Const reference to the current ConfigData.
	 */
	const ConfigData& getConfigDataRef() const { return configData; }


	/**********************************************************************************
	* Special async commands
	***********************************************************************************/
	/**
	 * @brief Checks if an asynchronous command is currently pending.
	 *
	 * @returns true if there is an active command awaiting an ACK,
	 *          false if the library is idle.
	 */
	bool asyncIsBusy();

	/**
	 * @brief Cancels any pending asynchronous command or sequence.
	 *
	 * If canceled, the callback of the running command is invoked
	 * with result type CANCELED. After canceling, the sensor may
	 * remain in config mode — consider disabling config mode or
	 * rebooting to return to detection operation.
	 */
	void asyncCancel();

	/**********************************************************************************
	* Config commands
	***********************************************************************************/

	/**
	 * @brief Enables config mode on the radar.
	 *
	 * Config mode must be enabled before issuing most configuration commands.
	 * This command itself is asynchronous — the callback fires once the
	 * sensor acknowledges the mode switch.
	 *
	 * @note If asyncIsBusy() is true, this command will not be sent.
	 * @note Normal detection data is suspended while config mode is active.
	 *
	 * @param callback Callback with signature
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 * @param userData Optional value that will be passed to the callback.
	 *
	 * @returns true if the command was sent, false if blocked.
	 */
	bool enableConfigModeAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	  * @brief Disables config mode on the radar.
	  *
	  * This should be called after finishing configuration, to return
	  * the sensor to normal detection operation.
	  *
	  * @note If an async command is already pending (asyncIsBusy() == true),
	  *       this command will not be sent.
	  *
	  * @param callback Callback with signature
	  *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	  * @param userData Optional value passed to the callback.
	  *
	  * @returns true if the command was sent, false otherwise.
	  */
	bool disableConfigModeAsync(AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Sets the maximum detection gates and "no-one" timeout.
	 *
	 * This command updates:
	 *   - Maximum motion detection distance gate (2–8).
	 *   - Maximum stationary detection distance gate (2–8).
	 *   - Timeout duration (0–65535 seconds) until "no presence" is declared.
	 *
	 * @note Requires config mode to be enabled. The method will internally
	 *       enable/disable config mode if necessary.
	 * @note Values outside the allowed ranges are clamped to valid limits.
	 *
	 * @param maxMovingGate Furthest gate used for motion detection (2–8).
	 * @param maxStationaryGate Furthest gate used for stationary detection (2–8).
	 * @param noOneTimeout Timeout in seconds until "no one" is reported (0–65535).
	 * @param callback Callback fired when ACK is received or on failure/timeout.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise (busy state).
	 */
	bool setMaxGateAndNoOneTimeoutAsync(byte maxMovingGate, byte maxStationaryGate, unsigned short noOneTimeout, AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Requests the current gate parameters from the sensor.
	 *
	 * Retrieves sensitivities, max gates, and timeout settings,
	 * which will be written into configData.
	 *
	 * @note Requires config mode. The method will manage mode switching if needed.
	 * @note If an async command is already pending, the request is rejected.
	 *
	 * @param callback Callback fired when data is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestGateParametersAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Enables engineering mode.
	 *
	 * In this mode, the sensor sends detailed per-gate signal values
	 * in addition to basic detection results.
	 *
	 * @note Engineering mode is temporary and lost after power cycle.
	 * @note Requires config mode. Will be enabled automatically if not active.
	 *
	 * @param callback Callback fired when ACK is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool enableEngineeringModeAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Disables engineering mode.
	 *
	 * Returns sensor reporting to basic detection results only.
	 *
	 * @note Requires config mode. Will be enabled automatically if not active.
	 *
	 * @param callback Callback fired when ACK is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool disableEngineeringModeAsync(AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Sets sensitivity thresholds for all gates at once.
	 *
	 * A sequence of commands will be sent, one for each gate.
	 * Threshold values are automatically clamped to 0–100.
	 *
	 * @note Requires config mode. Will be managed automatically.
	 * @note If another async command is pending, this call fails.
	 *
	 * @param movingThresholds Array of 9 sensitivity values for moving targets (0–100).
	 * @param stationaryThresholds Array of 9 sensitivity values for stationary targets (0–100).
	 * @param callback Callback fired when all updates are acknowledged or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the sequence was started, false otherwise.
	 */

	bool setDistanceGateSensitivityAsync(const byte movingThresholds[9], const byte stationaryThresholds[9], AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Sets sensitivity thresholds for a single gate.
	 *
	 * Updates both moving and stationary thresholds for the given gate index.
	 * If the gate index is greater than 8, all gates are updated instead.
	 *
	 * @note Threshold values are automatically clamped to 0–100.
	 * @note Requires config mode. Will be managed automatically.
	 *
	 * @param gate Index of the gate (0–8). Values >8 apply to all gates.
	 * @param movingThreshold Sensitivity for moving targets (0–100).
	 * @param stationaryThreshold Sensitivity for stationary targets (0–100).
	 * @param callback Callback fired when ACK is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool setDistanceGateSensitivityAsync(byte gate, byte movingThreshold, byte stationaryThreshold, AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Requests the firmware version of the sensor.
	 *
	 * Populates the firmware string when the ACK response arrives.
	 *
	 * @note Requires config mode. Will be managed automatically.
	 *
	 * @param callback Callback fired when firmware info is received.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestFirmwareAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Sets the UART baud rate of the sensor.
	 *
	 * The new baud rate becomes active only after reboot.
	 * The ESP32’s Serial interface must also be reconfigured
	 * to the new baud rate after reboot.
	 *
	 * @note Valid values are 1–8. Values outside range are rejected resp. method will fail.
	 * @note Requires config mode. Will be managed automatically.
	 *
	 * @param baudRateSetting Numeric setting:  1=9600, 2=19200, 3=38400, 4=57600, 5=115200, 6=230400, 7=25600 (factory default), 8=460800.
	 * @param callback Callback fired when ACK is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool setBaudRateAsync(byte baudRateSetting, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Sets the baudrate of the serial port of the sensor.
	*
	* The new baudrate will only become active after a reboot of the sensor.
	* If changing the baud rate, remember that you also need to addjust the baud rate of the ESP32 serial that is associated with then sensor.
	*
	* @param baudrate A valid baud rate from the Baudrate enum.
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool setBaudRateAsync(Baudrate baudRate, AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Restores factory settings of the sensor.
	 *
	 * Restored settings only become active after a reboot.
	 *
	 * @note Requires config mode. Will be managed automatically.
	 * @note After execution, call rebootAsync() to activate changes.
	 *
	 * @param callback Callback fired when ACK is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool restoreFactorySettingsAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Reboots the sensor.
	 *
	 * After reboot, the sensor stops responding for a few seconds.
	 * Config and engineering mode are reset.
	 *
	 * @note The reboot of the sensor takes place after the ACK has been sent.
	 *
	 * @param callback Callback fired when ACK is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool rebootAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Enables bluetooth
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool enableBluetoothAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Disables bluetooth
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool disableBluetoothAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Requests the bluetooth mac address
	*
	* @note The callback fires when the mac address has been received from the sensor (is sent with the ACK).
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool requestBluetoothMacAddressAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Sets the password for bluetooth access to the sensor.
	*
	* @param password New bluetooth password. Max 6. chars.
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool setBluetoothpasswordAsync(const char* password, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Sets the password for bluetooth access to the sensor.
	*
	* @param password New bluetooth password. Max 6. chars.
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool setBluetoothpasswordAsync(const String& password, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Resets the password for bluetooth access to the default value (HiLink)
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*/
	bool resetBluetoothpasswordAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Sets the distance resolution of the radar.
	 *
	 * The distance resolution defines the size of each distance gate
	 * and the maximum detection range:
	 *   - RESOLUTION_75CM → longer range, coarser detail.
	 *   - RESOLUTION_20CM → shorter range, finer detail.
	 *
	 * @note The new resolution takes effect immediately, but some
	 *       configuration-dependent values may require re-query.
	 * @note Requires config mode. Will be managed automatically.
	 *
	 * @param distanceResolution Value from the DistanceResolution enum.
	 *        Must not be NOT_SET.
	 * @param callback Function pointer with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when the ACK is received or on failure/timeout.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false if invalid parameters
	 *          or the library is busy.
	 */
	bool setDistanceResolutionAsync(DistanceResolution distanceResolution, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Sets the distance resolution explicitly to 75 cm per gate.
	*
	* Equivalent to setDistanceResolutionAsync(DistanceResolution::RESOLUTION_75CM).
	*
	* @param callback Function pointer with signature:
	*        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise.
	*/
	bool setDistanceResolution75cmAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Sets the distance resolution explicitly to 20 cm per gate.
	*
	* Equivalent to setDistanceResolutionAsync(DistanceResolution::RESOLUTION_20CM).
	*
	* @param callback Function pointer with signature:
	*        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise.
	*/
	bool setDistanceResolution20cmAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Requests the current distance resolution setting from the sensor.
	 *
	 * The result is written into configData.distanceResolution.
	 *
	 * @note Requires config mode. Will be managed automatically.
	 *
	 * @param callback Function pointer with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestDistanceResolutioncmAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Sets the auxiliary control parameters (light and output pin).
	 *
	 * This configures how the OUT pin behaves depending on light levels
	 * and presence detection. Typical use cases include controlling
	 * an external lamp or relay.
	 *
	 * @note Requires config mode. Will be managed automatically.
	 * @note Both enums must be set to valid values (not NOT_SET).
	 *
	 * @param lightControl Light control behavior (see LightControl enum).
	 * @param lightThreshold Threshold (0–255) used for light-based switching.
	 * @param outputControl Output pin logic configuration (see OutputControl enum).
	 * @param callback Function pointer with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when ACK is received or on failure/timeout.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool setAuxControlSettingsAsync(LightControl light_control, byte light_threshold, OutputControl output_control, AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Requests the current auxiliary control settings.
	 *
	 * Fills configData.lightControl, configData.lightThreshold,
	 * and configData.outputControl.
	 *
	 * @note Requires config mode. Will be managed automatically.
	 *
	 * @param callback Function pointer with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when ACK is received or on failure/timeout.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestAuxControlSettingsAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	 * @brief Requests all configuration parameters from the sensor.
	 *
	 * This triggers a sequence of queries that retrieves and updates:
	 *   - Gate parameters (sensitivities, max gates, timeout).
	 *   - Distance resolution setting.
	 *   - Auxiliary light/output control settings.
	 *
	 * The results are stored in configData, and the
	 * registerConfigUpdateReceivedCallback() is invoked after completion.
	 *
	 * @note Requires config mode. This method will manage mode switching automatically.
	 * @note If another async command is already pending, the request fails.
	 *
	 * ## Example: Refresh config data
	 * @code
	 *   radar.requestAllConfigData([](LD2410Async* sender,
	 *                                 AsyncCommandResult result,
	 *                                 byte) {
	 *     if (result == AsyncCommandResult::SUCCESS) {
	 *       Serial.println("All config data refreshed:");
	 *       sender->getConfigDataRef().print();
	 *     }
	 *   });
	 * @endcode
	 *
	 * ## Do:
	 * - Use this after connecting to ensure configData is fully populated.
	 * - Call before modifying config if you’re unsure of current values.
	 *
	 * ## Don’t:
	 * - Expect it to succeed if another async command is still running.
	 *
	 * @param callback Callback with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when all config data has been received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestAllConfigData(AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Requests all static information from the sensor.
	 *
	 * This includes:
	 *   - Firmware version string.
	 *   - Bluetooth MAC address (numeric and string form).
	 *
	 * The values are written into the public members `firmware`, `mac`,
	 * and `macString`.
	 *
	 * @note Requires config mode. Managed automatically by this method.
	 * @note If another async command is already pending, the request fails.
	 *
	 * ## Example: Retrieve firmware and MAC
	 * @code
	 *   radar.requestAllStaticData([](LD2410Async* sender,
	 *                                 AsyncCommandResult result,
	 *                                 byte) {
	 *     if (result == AsyncCommandResult::SUCCESS) {
	 *       Serial.print("Firmware: ");
	 *       Serial.println(sender->firmware);
	 *
	 *       Serial.print("MAC: ");
	 *       Serial.println(sender->macString);
	 *     }
	 *   });
	 * @endcode
	 *
	 * ## Do:
	 * - Use after initialization to log firmware version and MAC.
	 * - Useful for debugging or inventory identification.
	 *
	 * ## Don’t:
	 * - Expect frequently changing data — this is static information.
	 *
	 * @param callback Callback with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when static data is received or on failure.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestAllStaticData(AsyncCommandCallback callback, byte userData = 0);



	/**
	 * @brief Applies a full ConfigData struct to the sensor.
	 *
	 * The provided ConfigData is converted into the appropriate sequence
	 * of configuration commands and sent asynchronously. This allows
	 * bulk updating of multiple settings (gate sensitivities, timeouts,
	 * resolution, auxiliary controls) in one call.
	 *
	 * @note Requires config mode. This method will manage entering and
	 *       exiting config mode automatically.
	 * @note Any members of ConfigData that are left at invalid values
	 *       (e.g. enums set to NOT_SET) will cause the sequence to fail.
	 *
	 * ## Example: Clone, modify, and apply config
	 * @code
	 *   ConfigData cfg = radar.getConfigData();  // clone current config
	 *   cfg.noOneTimeout = 120;                  // change timeout
	 *   cfg.distanceGateMotionSensitivity[2] = 75;
	 *
	 *   radar.setConfigDataAsync(cfg, [](LD2410Async* sender,
	 *                                    AsyncCommandResult result,
	 *                                    byte) {
	 *     if (result == AsyncCommandResult::SUCCESS) {
	 *       Serial.println("All config applied successfully!");
	 *     }
	 *   });
	 * @endcode
	 *
	 * ## Do:
	 * - Use this for bulk updates of sensor configuration.
	 * - Always start from a clone of getConfigData() to avoid missing fields.
	 *
	 * ## Don’t:
	 * - Pass uninitialized or partially filled ConfigData (may fail).
	 * - Expect changes to persist after power cycle without reapplying.
	 *
	 * @param config   The configuration data to be applied.
	 * @param callback Function with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData),
	 *        executed when the sequence finishes (success/fail/timeout/cancel).
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command sequence has been started, false otherwise.
	 */
	bool setConfigDataAsync(const ConfigData& config, AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Starts the automatic configuration (auto-config) routine on the sensor.
	 *
	 * Auto-config lets the radar adjust its internal thresholds and
	 * sensitivities for the current environment. This can take several
	 * seconds to complete and results in updated sensitivity values.
	 *
	 * The progress and result can be checked with requestAutoConfigStatusAsync().
	 *
	 * @note Requires config mode. This method will manage entering and
	 *       exiting config mode automatically.
	 * @note Auto-config temporarily suspends normal detection reporting.
	 *
	 * ## Example: Run auto-config
	 * @code
	 *   radar.beginAutoConfigAsync([](LD2410Async* sender,
	 *                                 AsyncCommandResult result,
	 *                                 byte) {
	 *     if (result == AsyncCommandResult::SUCCESS) {
	 *       Serial.println("Auto-config started.");
	 *     } else {
	 *       Serial.println("Failed to start auto-config.");
	 *     }
	 *   });
	 * @endcode
	 *
	 * ## Do:
	 * - Use in new environments to optimize detection performance.
	 * - Query status afterwards with requestAutoConfigStatusAsync().
	 *
	 * ## Don’t:
	 * - Expect instant results — the sensor needs time to complete the process.
	 *
	 * @param callback Callback with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when the command is acknowledged or on failure/timeout.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool beginAutoConfigAsync(AsyncCommandCallback callback, byte userData = 0);


	/**
	 * @brief Requests the current status of the auto-config routine.
	 *
	 * The status is written into the member variable autoConfigStatus:
	 *   - NOT_IN_PROGRESS → no auto-config running.
	 *   - IN_PROGRESS → auto-config is currently running.
	 *   - COMPLETED → auto-config finished (success or failure).
	 *
	 * @note Requires config mode. This method will manage mode switching automatically.
	 * @note If another async command is already pending, this call fails.
	 *
	 * ## Example: Check auto-config status
	 * @code
	 *   radar.requestAutoConfigStatusAsync([](LD2410Async* sender,
	 *                                         AsyncCommandResult result,
	 *                                         byte) {
	 *     if (result == AsyncCommandResult::SUCCESS) {
	 *       switch (sender->autoConfigStatus) {
	 *         case AutoConfigStatus::NOT_IN_PROGRESS:
	 *           Serial.println("Auto-config not running.");
	 *           break;
	 *         case AutoConfigStatus::IN_PROGRESS:
	 *           Serial.println("Auto-config in progress...");
	 *           break;
	 *         case AutoConfigStatus::COMPLETED:
	 *           Serial.println("Auto-config completed.");
	 *           break;
	 *         default:
	 *           Serial.println("Unknown auto-config status.");
	 *       }
	 *     } else {
	 *       Serial.println("Failed to request auto-config status.");
	 *     }
	 *   });
	 * @endcode
	 *
	 * ## Do:
	 * - Use this after beginAutoConfigAsync() to track progress.
	 * - Use autoConfigStatus for decision-making in your logic.
	 *
	 * ## Don’t:
	 * - Assume COMPLETED means success — thresholds should still be verified.
	 *
	 * @param callback Callback with signature:
	 *        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	 *        Fired when the sensor replies or on failure/timeout.
	 * @param userData Optional value passed to the callback.
	 *
	 * @returns true if the command was sent, false otherwise.
	 */
	bool requestAutoConfigStatusAsync(AsyncCommandCallback callback, byte userData = 0);


};
