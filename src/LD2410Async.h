#pragma once


#include "Arduino.h"
#include "Ticker.h"
#include "LD2410Debug.h"
#include "LD2410Types.h"
#include "LD2410Defs.h"





/**
 * @brief Asynchronous driver for the LD2410 human presence radar sensor.
 *
 * @mainpage
 * @section intro_sec Introduction
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
 *   radar.configureAllConfigSettingsAsync(cfg, [](LD2410Async* sender,
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
	 * @brief Result of an asynchronous command execution.
	 *
	 * Every async command reports back its outcome via the callback.
	 *
	 */
	enum class AsyncCommandResult : byte {
		SUCCESS,    ///< Command completed successfully and ACK was received.
		FAILED,     ///< Command failed (sensor responded with negative ACK).
		TIMEOUT,    ///< No ACK received within the expected time window.
		CANCELED    ///< Command was canceled by the user before completion.
	};




	/**
	 * @brief Callback signature for asynchronous command completion.
	 *
	 * @param sender   Pointer to the LD2410Async instance that triggered the callback.
	 * @param result   Outcome of the async operation (SUCCESS, FAILED, TIMEOUT, CANCELED).
	 * @param userData User-specified value passed when registering the callback.
	 *
	 */
	typedef void (*AsyncCommandCallback)(LD2410Async* sender, AsyncCommandResult result, byte userData);

	/**
	 * @brief Generic callback signature used for simple notifications.
	 *
	 * @param sender   Pointer to the LD2410Async instance that triggered the callback.
	 * @param userData User-specified value passed when registering the callback.
	 *
	 *
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
	 *
	 */
	typedef void(*DetectionDataCallback)(LD2410Async* sender, bool presenceDetected, byte userData);





public:



	/**
	* @brief Latest detection results from the radar.
	*
	* @details Updated automatically whenever new data frames are received.
	* Use registerDetectionDataReceivedCallback() to be notified
	* whenever this struct changes.
	* Use getDetectionData() or getDetectionDataRef() to access the current values, rather than accessing the struct directly.
	*
	*
	*/
	LD2410Types::DetectionData detectionData;

	/**
	* @brief Current configuration parameters of the radar.
	*
	* @details Filled when configuration query commands are issued
	* (e.g. requestAllConfigSettingsAsync() or requestGateParametersAsync() ect).
	* Use registerConfigUpdateReceivedCallback() to be notified when data in this struct changes.
	* Use getConfigData() or getConfigDataRef() to access the current values, rather than accessing the struct directly.
	*
	* Structure will contain only uninitilaized data if config data is not queried explicitly.
	*
	*/
	LD2410Types::ConfigData configData;

	/**
	* @brief Static data of the radar
	* 
	* @details Filled when config mode is being enabled (protocol version and buffer size)
	* annd when issuing query commands for the static data (@ref requestAllStaticDataAsync, @ref requestFirmwareAsync, @ref requestBluetoothMacAddressAsync)
	*/
	LD2410Types::StaticData staticData;



	/**
	* @brief True if the sensor is currently in config mode.
	*
	* Config mode must be enabled using enableConfigModeAsync() before sending configuration commands.
	* After sending config commands, always disable the config mode using disableConfigModeAsync(),
	* otherwiese the radar will not send any detection data.
	*
	*/
	bool configModeEnabled = false;


	/**
	* @brief True if the sensor is currently in engineering mode.
	*
	* In engineering mode, the radar sends detailed per-gate
	* signal data in addition to basic detection data.
	*
	* Use enableEngineeringModeAsync() and disableEngineeringModeAsync() to control the engineering mode.
	*
	*/
	bool engineeringModeEnabled = false;




	/**
	* @brief Current status of the auto-configuration routine.
	*
	* Updated by requestAutoConfigStatusAsync().
	*
	*/
	LD2410Types::AutoConfigStatus	autoConfigStatus = LD2410Types::AutoConfigStatus::NOT_SET;




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
	* 
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
	* 
	*/
	bool begin();

	/**
	* @brief Stops the background task started by begin().
	*
	* After calling end(), no more data will be processed until begin() is called again.
	* This is useful to temporarily suspend radar processing without rebooting.
	*
	* @returns true if the task was stopped, false if it was not active.
	* 
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
	* 
	*/
	void setInactivityHandling(bool enable);

	/**
	* @brief Convenience method: enables inactivity handling.
	*
	* Equivalent to calling setInactivityHandling(true).
	* 
	*/
	void enableInactivityHandling() { setInactivityHandling(true); };

	/**
	* @brief Convenience method: disables inactivity handling.
	*
	* Equivalent to calling setInactivityHandling(false).
	* 
	*/
	void disableInactivityHandling() { setInactivityHandling(false); };

	/**
	* @brief Returns whether inactivity handling is currently enabled.
	*
	* @returns true if inactivity handling is enabled, false otherwise.
	* 
	*/
	bool isInactivityHandlingEnabled() const { return inactivityHandlingEnabled; };

	/**
	* @brief Sets the timeout period for inactivity handling.
	*
	* If no data or command ACK is received within this period,
	* the library will attempt to recover the sensor as described
	* in setInactivityHandling().
	*
	* Default is 60000 ms (1 minute).
	*
	* @param timeoutMs Timeout in milliseconds (minimum 10000 ms recommended). 0 will diable inactivity checking and handling.
	* 
	*/
	void setInactivityTimeoutMs(unsigned long timeoutMs) { inactivityHandlingTimeoutMs = timeoutMs; };

	/**
	* @brief Returns the current inactivity timeout period.
	*
	* @returns Timeout in milliseconds.
	* 
	*/
	unsigned long getInactivityTimeoutMs() const { return inactivityHandlingTimeoutMs; };



	/**********************************************************************************
	* Callback registration methods
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
	* 
	*/
	void registerDetectionDataReceivedCallback(DetectionDataCallback callback, byte userData = 0);

	/**
	* @brief Registers a callback for configuration changes.
	*
	* The callback is invoked whenever the sensor’s configuration
	* has been successfully updated (e.g. after setting sensitivity).
	*
	* @param callback Function pointer with signature
	*        void methodName(LD2410Async* sender, byte userData).
	* @param userData Optional value that will be passed to the callback.
	* 
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
	* 
	*/
	void registerConfigUpdateReceivedCallback(GenericCallback callback, byte userData = 0);



	/**********************************************************************************
	* Detection and config data access commands
	***********************************************************************************/
	// It is recommended to use the data access commands instead of accessing the detectionData and the configData structs in the class directly.

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
	* 
	*/
	LD2410Types::DetectionData getDetectionData() const;


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
	* 
	*/
	const LD2410Types::DetectionData& getDetectionDataRef() const { return detectionData; }


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
	*   radar.configureAllConfigSettingsAsync(cfg, [](LD2410Async* sender,
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
	* 
	*/
	LD2410Types::ConfigData getConfigData() const;


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
	* 
	*/
	const LD2410Types::ConfigData& getConfigDataRef() const { return configData; }


	/**********************************************************************************
	* Special async commands
	***********************************************************************************/
	/**
	* @brief Checks if an asynchronous command is currently pending.
	*
	* @returns true if there is an active command awaiting an ACK,
	*          false if the library is idle.
	* 
	*/
	bool asyncIsBusy();

	/**
	* @brief Cancels any pending asynchronous command or sequence.
	*
	* If canceled, the callback of the running command is invoked
	* with result type CANCELED. After canceling, the sensor may
	* remain in config mode — consider disabling config mode or
	* rebooting to return to detection operation.
	* 
	*/
	void asyncCancel();

	/**
	* @brief Sets the timeout for async command callbacks.
	*
	* #note Make sure the timeout is long enough to allow for the execution of long running commands. In partuclar enabling config mode can take up to 6 secs.
	*
	*
	* @param timeoutMs Timeout in milliseconds (default 6000 ms).
	* 
	*/
	void setAsyncCommandTimeoutMs(unsigned long timeoutMs) { asyncCommandTimeoutMs = timeoutMs; }

	/**
	* @brief Returns the current async command timeout.
	*
	* @return Timeout in milliseconds.
	* 
	*/
	unsigned long getAsyncCommandTimeoutMs() const { return asyncCommandTimeoutMs; }


	/**********************************************************************************
	* Commands
	***********************************************************************************/

	/*---------------------------------------------------------------------------------
	- Config mode commands
	---------------------------------------------------------------------------------*/
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
	* 
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
	*
	*/
	bool disableConfigModeAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Detects if config mode is enabled
	*
	* @returns true if config mode is anabled, false if config mode is disabled
	*
	*/
	bool isConfigModeEnabled() const {
	return configModeEnabled;
	};


	/*---------------------------------------------------------------------------------
	- Engineering mode commands
	---------------------------------------------------------------------------------*/
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
	*
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
	*
	*/
	bool disableEngineeringModeAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Detects if engineering mode is enabled
	*
	* @returns true if engineering mode is anabled, false if engineering mode is disabled
	* 
	*/
	bool isEngineeringModeEnabled() const {
	return engineeringModeEnabled;
	};

	/*---------------------------------------------------------------------------------
	- Native sensor commands
	---------------------------------------------------------------------------------*/
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
	*
	*/
	bool requestGateParametersAsync(AsyncCommandCallback callback, byte userData = 0);



	/**
	* @brief Configures the maximum detection gates and "no-one" timeout on the sensor.
	*
	* This command updates:
	*   - Maximum motion detection distance gate (2–8).
	*   - Maximum stationary detection distance gate (2–8).
	*   - Timeout duration (0–65535 seconds) until "no presence" is declared.
	*
	* @note Requires config mode to be enabled. The method will internally
	*       enable/disable config mode if necessary.
	* @note If another async command is pending, this call fails.
	*
	* @param maxMovingGate Furthest gate used for motion detection (2–8).
	* @param maxStationaryGate Furthest gate used for stationary detection (2–8).
	* @param noOneTimeout Timeout in seconds until "no one" is reported (0–65535).
	* @param callback Callback fired when ACK is received or on failure/timeout.
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise (busy state or invalid values).
	*
	*/
	bool configureMaxGateAndNoOneTimeoutAsync(byte maxMovingGate, byte maxStationaryGate, unsigned short noOneTimeout, AsyncCommandCallback callback, byte userData = 0);


	/**
	* @brief Configures sensitivity thresholds for all gates at once.
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
	*
	*/

	bool configureDistanceGateSensitivityAsync(const byte movingThresholds[9], const byte stationaryThresholds[9], AsyncCommandCallback callback, byte userData = 0);


	/**
	* @brief Configures sensitivity thresholds for a single gate.
	*
	* Updates both moving and stationary thresholds for the given gate index.
	* If the gate index is greater than 8, all gates are updated instead.
	*
	* @note Requires config mode. Will be managed automatically.
	* @note If another async command is pending, this call fails.
	*
	* @param gate Index of the gate (0–8). Values >8 apply to all gates.
	* @param movingThreshold Sensitivity for moving targets (0–100).
	* @param stationaryThreshold Sensitivity for stationary targets (0–100).
	* @param callback Callback fired when ACK is received or on failure.
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise.
	* 
	*
	*/
	bool configureDistanceGateSensitivityAsync(byte gate, byte movingThreshold, byte stationaryThreshold, AsyncCommandCallback callback, byte userData = 0);

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
	*
	*/
	bool requestFirmwareAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Configures the UART baud rate of the sensor.
	*
	* The new baud rate becomes active only after reboot.
	* The ESP32’s Serial interface must also be reconfigured
	* to the new baud rate after reboot.
	*
	* @note Valid values are 1–8. Values outside range are rejected resp. method will fail.
	* @note Requires config mode. Will be managed automatically.
	* @note If another async command is pending, this call fails.
	* @note After execution, call rebootAsync() to activate changes.
	*
	* @param baudRateSetting Numeric setting:  1=9600, 2=19200, 3=38400, 4=57600, 5=115200, 6=230400, 7=25600 (factory default), 8=460800.
	* @param callback Callback fired when ACK is received or on failure.
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise.
	*
	*/
	bool configureBaudRateAsync(byte baudRateSetting, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Configures the baudrate of the serial port of the sensor.
	*
	* The new baudrate will only become active after a reboot of the sensor.
	* If changing the baud rate, remember that you also need to addjust the baud rate of the ESP32 serial that is associated with then sensor.
	*
	* @note If another async command is pending, this call fails.
	* @note After execution, call rebootAsync() to activate changes.
	*
	* @param baudrate A valid baud rate from the Baudrate enum.
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*
	*/
	bool configureBaudRateAsync(LD2410Types::Baudrate baudRate, AsyncCommandCallback callback, byte userData = 0);


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
	*
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
	*
		*/
	bool rebootAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Enables bluetooth
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*
		*/
	bool enableBluetoothAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Disables bluetooth
	*
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*
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
	*
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
	*
	*/
	bool configureBluetoothPasswordAsync(const char* password, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Sets the password for bluetooth access to the sensor.
	*
	* @param password New bluetooth password. Max 6. chars.
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*
	*/
	bool configureBluetoothPasswordAsync(const String& password, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Resets the password for bluetooth access to the default value (HiLink)
	* @param callback Callback method with void methodName(LD2410Async* sender, AsyncCommandResult result, byte userData) signature. Will be called after the Ack for the command has been received (success=true) or after the command timeout (success=false) or after the command has been canceld (sucess=false).
	* @param userData Optional value that will be passed to the callback function.
	*
	* @returns true if the command has been sent, false if the command cant be sent (typically because another async command is pending).
	*
	*/
	bool configureDefaultBluetoothPasswordAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Configures the distance resolution of the radar.
	*
	* The distance resolution defines the size of each distance gate
	* and the maximum detection range:
	*   - RESOLUTION_75CM → longer range, coarser detail.
	*   - RESOLUTION_20CM → shorter range, finer detail.
	*
	* @note Requires config mode. Will be managed automatically.
	* @note Requires a reboot to activate value changes. Call rebootAsync() after setting.
	* @note Fails if another async command is pending.
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
	*
	*/
	bool configureDistanceResolutionAsync(LD2410Types::DistanceResolution distanceResolution, AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Configures the distance resolution explicitly to 75 cm per gate.
	*
	* Equivalent to configureDistanceResolutionAsync(DistanceResolution::RESOLUTION_75CM).
	*
	* @note Requires config mode. Will be managed automatically.
	* @note Requires a reboot to activate value changes. Call rebootAsync() after setting.
	* @note Fails if another async command is pending.
	*
	* @param callback Function pointer with signature:
	*        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise.
	*
	*/
	bool configureDistanceResolution75cmAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Configures the distance resolution explicitly to 20 cm per gate.
	*
	* Equivalent to configureDistanceResolutionAsync(DistanceResolution::RESOLUTION_20CM).
	*
	* @note Requires config mode. Will be managed automatically.
	* @note Requires a reboot to activate value changes. Call rebootAsync() after setting.
	* @note Fails if another async command is pending.
	*
	* @param callback Function pointer with signature:
	*        void(LD2410Async* sender, AsyncCommandResult result, byte userData).
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command was sent, false otherwise.
	*
	*/
	bool configuresDistanceResolution20cmAsync(AsyncCommandCallback callback, byte userData = 0);

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
	*
	*/
	bool requestDistanceResolutioncmAsync(AsyncCommandCallback callback, byte userData = 0);

	/**
	* @brief Configures the auxiliary control parameters (light and output pin).
	*
	* This configures how the OUT pin behaves depending on light levels
	* and presence detection. Typical use cases include controlling
	* an external lamp or relay.
	*
	* @note Requires config mode. Will be managed automatically.
	* @note Both enums must be set to valid values (not NOT_SET).
	* @note Fails if another async command is pending.
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
	*
	*/
	bool configureAuxControlSettingsAsync(LD2410Types::LightControl light_control, byte light_threshold, LD2410Types::OutputControl output_control, AsyncCommandCallback callback, byte userData = 0);

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
	*
	*/
	bool requestAuxControlSettingsAsync(AsyncCommandCallback callback, byte userData = 0);


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
	*
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
	*
	*/
	bool requestAutoConfigStatusAsync(AsyncCommandCallback callback, byte userData = 0);



	/*---------------------------------------------------------------------------------
	- High level commands
	---------------------------------------------------------------------------------*/
	// High level commands typically encapsulate several native commands.
	// They provide a more consistent access to the sensors configuration,
	// e.g. for reading all config data 3 native commands are necessary,
	// to update the same data up to 12 native commands may be required.
	//The highlevel commands encapsulate both situation into a single command

	/**
	* @brief Requests all configuration settings from the sensor.
	*
	* This triggers a sequence of queries that retrieves and updates:
	*   - Gate parameters (sensitivities, max gates, timeout).
	*   - Distance resolution setting.
	*   - Auxiliary light/output control settings.
	*
	* The results are stored in configData, and the
	* registerConfigUpdateReceivedCallback() is invoked after completion.
	*
	* @note This is a high-level method that involves multiple commands.
	* @note Requires config mode. This method will manage mode switching automatically.
	* @note If another async command is already pending, the request fails.
	*
	* ## Example: Refresh config data
	* @code
	*   radar.requestAllConfigSettingsAsync([](LD2410Async* sender,
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
	*
	*/
	bool requestAllConfigSettingsAsync(AsyncCommandCallback callback, byte userData = 0);


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
	* @note This is a high-level method that involves multiple commands.
	* @note Requires config mode. Managed automatically by this method.
	* @note If another async command is already pending, the request fails.
	*
	* ## Example: Retrieve firmware and MAC
	* @code
	*   radar.requestAllStaticDataAsync([](LD2410Async* sender,
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
	* 
	*/
	bool requestAllStaticDataAsync(AsyncCommandCallback callback, byte userData = 0);



	/**
	* @brief Applies a full ConfigData struct to the LD2410.
	*
	* If writeAllConfigData is true, the method will first fetch the current config,
	* compare it with the provide Config data and then create a command sequence that
	* will only update the changes config values.
	* If writeAllConfigData is false, the method will write all values in the provided
	* ConfigData to the sensor, regardless of whether they differ from the current config.
	*
	* @note This is a high-level method that involves multiple commands (up to 18).
	* @note Requires config mode. This method will manage entering and
	*       exiting config mode automatically (if config mode is not already active).
	* @note If another async command is already pending, the command fails.
	* @note Any members of ConfigData that are left at invalid values
	*       (e.g. enums set to NOT_SET) will cause the sequence to fail.
	*
	* ## Example: Clone, modify, and apply config
	* @code
	*   ConfigData cfg = radar.getConfigData();  // clone current config
	*   cfg.noOneTimeout = 120;                  // change timeout
	*   cfg.distanceGateMotionSensitivity[2] = 75;
	*
	*   radar.configureAllConfigSettingsAsync(cfg, [](LD2410Async* sender,
	*                                    AsyncCommandResult result,
	*                                    byte) {
	*     if (result == AsyncCommandResult::SUCCESS) {
	*       Serial.println("All config applied successfully!");
	*     }
	*   });
	* @endcode
	*
	* ## Do:
	* - Always start from a clone of getConfigData() to avoid settings unwanted values for settings that dont need to change.
	* - If the methods callback does not report SUCCESS, any portion of the config might have been written to the sensor or the sensor might has remained in config mode.
	*   Make sure to take appropriate measures to return the sensor to normal operaion mode if required (reboot usually does the trick) and check the config values, if they are what you need.
	*
	* ## Don’t:
	* - Dont write all config data (writeAllConfigData=true) if not really necessary.
	*   This generates unnecessary wear on the sensors memory.
	* - Pass uninitialized or partially filled ConfigData (may fail or result in unwanted settings)

	*
	* @param configToWrite The configuration data to be applied.
	* @param writeAllConfigData If true, all fields in configToWrite are applied. If false, changed values are written.
	* @param callback Function with signature:
	*        void(LD2410Async* sender, AsyncCommandResult result, byte userData),
	*        executed when the sequence finishes (success/fail/timeout/cancel).
	* @param userData Optional value passed to the callback.
	*
	* @returns true if the command sequence has been started, false otherwise.
	* 
	*/
	bool configureAllConfigSettingsAsync(const LD2410Types::ConfigData& configToWrite, bool writeAllConfigData, AsyncCommandCallback callback, byte userData = 0);



	private:
	// ============================================================================
	// Low-level serial interface
	// ============================================================================

	/// Pointer to the Serial/Stream object connected to the LD2410 sensor
	Stream* sensor;


	// ============================================================================
	// Frame parsing state machine
	// ============================================================================

	/// States used when parsing incoming frames from the sensor
	enum ReadFrameState {
	WAITING_FOR_HEADER,   ///< Waiting for frame header sequence
	ACK_HEADER,           ///< Parsing header of an ACK frame
	DATA_HEADER,          ///< Parsing header of a DATA frame
	READ_ACK_SIZE,        ///< Reading payload size field of an ACK frame
	READ_DATA_SIZE,       ///< Reading payload size field of a DATA frame
	READ_ACK_PAYLOAD,     ///< Reading payload content of an ACK frame
	READ_DATA_PAYLOAD     ///< Reading payload content of a DATA frame
	};

	/// Result type when trying to read a frame
	enum FrameReadResponse {
	FAIL = 0, ///< Frame was invalid or incomplete
	ACK,      ///< A valid ACK frame was received
	DATA      ///< A valid DATA frame was received
	};

	int readFrameHeaderIndex = 0; ///< Current index while matching the frame header
	int payloadSize = 0;          ///< Expected payload size of current frame
	ReadFrameState readFrameState = ReadFrameState::WAITING_FOR_HEADER; ///< Current frame parsing state

	/// Extract payload size from the current byte and update state machine
	bool readFramePayloadSize(byte b, ReadFrameState nextReadFrameState);

	/// Read payload bytes until full ACK/DATA frame is assembled
	FrameReadResponse readFramePayload(byte b, const byte* tailPattern, LD2410Async::FrameReadResponse succesResponseType);

	/// State machine entry: read incoming frame and return read response
	FrameReadResponse readFrame();


	// ============================================================================
	// Receive buffer
	// ============================================================================

	/// Raw buffer for storing incoming bytes
	byte receiveBuffer[LD2410Defs::LD2410_Buffer_Size];

	/// Current index into receiveBuffer
	byte receiveBufferIndex = 0;


	// ============================================================================
	// Asynchronous command sequence handling
	// ============================================================================

	/// Maximum number of commands in one async sequence
	static constexpr size_t MAX_COMMAND_SEQUENCE_LENGTH = 15;

	/// Buffer holding queued commands for sequence execution
	byte commandSequenceBuffer[MAX_COMMAND_SEQUENCE_LENGTH][LD2410Defs::LD2410_Buffer_Size];

	/// Number of commands currently queued in the sequence buffer
	byte commandSequenceBufferCount = 0;

	/// Timestamp when the current async sequence started
	unsigned long executeCommandSequenceStartMs = 0;

	/// Callback for current async sequence
	AsyncCommandCallback executeCommandSequenceCallback = nullptr;

	/// User-provided data passed to async sequence callback
	byte executeCommandSequenceUserData = 0;

	/// True if an async sequence is currently pending.
	bool executeCommandSequenceActive = false;

	/// Ticker used for small delay before firing the commandsequence callback when the command sequence is empty.
	/// This ensures that the callback is always called asynchronously and never directly from the calling context.
	Ticker executeCommandSequenceOnceTicker;

	/// Index of currently active command in the sequence buffer
	int executeCommandSequenceIndex = 0;

	/// Stores config mode state before sequence started (to restore later)
	bool executeCommandSequenceInitialConfigModeState = false;

	/// Finalize an async sequence and invoke its callback
	void executeCommandSequenceAsyncExecuteCallback(LD2410Async::AsyncCommandResult result);

	/// Final step of an async sequence: restore config mode if needed and call callback
	void executeCommandSequenceAsyncFinalize(LD2410Async::AsyncCommandResult resultToReport);

	/// Internal callbacks for sequence steps
	static void executeCommandSequenceAsyncDisableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData = 0);
	static void executeCommandSequenceAsyncCommandCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData = 0);

	/// Start executing an async sequence
	bool executeCommandSequenceAsync(AsyncCommandCallback callback, byte userData = 0);

	/// Add one command to the sequence buffer
	bool addCommandToSequence(const byte* command);

	/// Reset sequence buffer to empty
	bool resetCommandSequence();


	// ============================================================================
	// Inactivity handling
	// ============================================================================

	/// Update last-activity timestamp ("I am alive" signal)
	void heartbeat();

	bool inactivityHandlingEnabled = true;     ///< Whether automatic inactivity handling is active
	unsigned long inactivityHandlingTimeoutMs = 60000; ///< Timeout until recovery action is triggered (ms)
	unsigned long lastActivityMs = 0;          ///< Timestamp of last received activity
	bool handleInactivityExitConfigModeDone = false; ///< Flag to avoid repeating config-mode exit

	/// Main inactivity handler: exit config mode or reboot if stuck
	void handleInactivity();

	/// Callback for reboot triggered by inactivity handler
	static void handleInactivityRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData);

	/// Callback for disabling config mode during inactivity recovery
	static void handleInactivityDisableConfigmodeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData);


	// ============================================================================
	// Reboot handling
	// ============================================================================

	/// Step 1: Enter config mode before reboot
	static void rebootEnableConfigModeCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData = 0);

	/// Step 2: Issue reboot command
	static void rebootRebootCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData = 0);


	// ============================================================================
	// ACK/DATA processing
	// ============================================================================

	/// Process a received ACK frame
	bool processAck();

	/// Process a received DATA frame
	bool processData();


	// ============================================================================
	// Callbacks
	// ============================================================================

	GenericCallback configUpdateReceivedReceivedCallback = nullptr; ///< Callback for new config data received
	byte configUpdateReceivedReceivedCallbackUserData = 0;          ///< UserData for config-update callback
	void executeConfigUpdateReceivedCallback();                     ///< Execute config-update callback

	GenericCallback configChangedCallback = nullptr; ///< Callback for successful config change
	byte configChangedCallbackUserData = 0;          ///< UserData for config-changed callback
	void executeConfigChangedCallback();             ///< Execute config-changed callback

	DetectionDataCallback detectionDataCallback = nullptr; ///< Callback for new detection data
	byte detectionDataCallbackUserData = 0;                ///< UserData for detection-data callback


	// ============================================================================
	// Command sending
	// ============================================================================

	/// Send raw command bytes to the sensor
	void sendCommand(const byte* command);


	// ============================================================================
	// FreeRTOS task management
	// ============================================================================

	TaskHandle_t taskHandle = NULL; ///< Handle to FreeRTOS background task
	bool taskStop = false;          ///< Stop flag for task loop
	void taskLoop();                ///< Background task loop for reading data


	// ============================================================================
	// Async command handling
	// ============================================================================

	///< Timeout for async commands in ms (default 6000).
	unsigned long asyncCommandTimeoutMs = 6000;

	/// Send a generic async command
	bool sendCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData = 0);

	/// Invoke async command callback with result
	void executeAsyncCommandCallback(byte commandCode, LD2410Async::AsyncCommandResult result);



	/// Handle async command timeout
	void handleAsyncCommandCallbackTimeout();

	/// Send async command that modifies configuration
	bool sendConfigCommandAsync(const byte* command, AsyncCommandCallback callback, byte userData = 0);

	AsyncCommandCallback asyncCommandCallback = nullptr; ///< Current async command callback
	byte asyncCommandCallbackUserData = 0;               ///< UserData for current async command
	unsigned long asyncCommandStartMs = 0;               ///< Timestamp when async command started
	byte asyncCommandCommandCode = 0;                    ///< Last command code issued
	bool asyncCommandActive = false;					 ///< True if an async command is currently pending.


	// ============================================================================
	// Data processing
	// ============================================================================

	/// Main dispatcher: process incoming bytes into frames, update state, trigger callbacks
	void processReceivedData();

	// ============================================================================
	// Config mode 
	// ============================================================================
	bool enableConfigModeInternalAsync(AsyncCommandCallback callback, byte userData);
	bool disableConfigModeInternalAsync(AsyncCommandCallback callback, byte userData);

	// ============================================================================
	// Config writing 
	// ============================================================================
	LD2410Types::ConfigData configureAllConfigSettingsAsyncConfigDataToWrite;
	bool configureAllConfigSettingsAsyncWriteFullConfig = false;
	AsyncCommandCallback configureAllConfigSettingsAsyncConfigCallback = nullptr;
	byte configureAllConfigSettingsAsyncConfigUserData = 0;
	bool configureAllConfigSettingsAsyncConfigActive = false;
	bool configureAllConfigSettingsAsyncConfigInitialConfigMode = false;
	AsyncCommandResult configureAllConfigSettingsAsyncResultToReport = LD2410Async::AsyncCommandResult::SUCCESS;

	void configureAllConfigSettingsAsyncExecuteCallback(LD2410Async::AsyncCommandResult result);
	static	void configureAllConfigSettingsAsyncConfigModeDisabledCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData);
	void configureAllConfigSettingsAsyncFinalize(LD2410Async::AsyncCommandResult resultToReport);
	static void configureAllConfigSettingsAsyncWriteConfigCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData);
	bool configureAllConfigSettingsAsyncWriteConfig();
	static void configureAllConfigSettingsAsyncRequestAllConfigDataCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData);
	bool configureAllConfigSettingsAsyncRequestAllConfigData();
	static void configureAllConfigSettingsAsyncConfigModeEnabledCallback(LD2410Async* sender, LD2410Async::AsyncCommandResult result, byte userData);

	bool configureAllConfigSettingsAsyncBuildSaveChangesCommandSequence();

};
