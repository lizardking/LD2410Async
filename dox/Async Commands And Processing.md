@page Async_Commands_And_Processing Async Commands & Processing
# Async Commands & Processing

## Why asynchronous processing matters for the LD2410

The LD2410 radar sensor continuously streams detection frames and requires explicit configuration commands to adjust its behavior. 
If the library were implemented with blocking (synchronous) calls, every command would have to wait for the sensor’s response before the application could continue. 
Since enabling config mode or requesting data can take several seconds, blocking calls would freeze the main program loop and would disrupt other tasks.

An asynchronous approach avoids this problem. Commands are sent non-blocking, and the library reports back via callbacks when acknowledgements or data arrive. This has several benefits:
- Responsiveness - the main loop and other tasks keep running while waiting for the sensor.
- Efficiency - incoming data frames are parsed in the background without polling or busy-waiting.
- Scalability - multiple sensors or other asynchronous components (Wi-Fi, MQTT, UI updates) can run in parallel without interfering with each other.
- Robustness - timeouts and retries can be handled cleanly without stalling the system.

## Detection Data Callback

Whenever the LD2410 sensor transmits a valid data frame, the library automatically parses it and invokes the detection data callback. This allows your application to react immediately to sensor input without the need for polling.

Use @ref LD2410Async::onDetectionDataReceived "onDetectionDataReceived()" to register a callback function for detection events.

The callback delivers two parameters:
- A pointer to the LD2410Async instance that triggered the event. This allows convenient access to members of that instance.
- A simple presenceDetected flag (true if presence is detected, false otherwise).

@note 
The callback is triggered for every received frame, not only when presenceDetected changes. This means you can always rely on it to reflect the most recent sensor state.

For applications that need more than just the quick presence flag, the library provides full access to the updated @ref LD2410Types::DetectionData "DetectionData" struct. This struct contains distances, signal strengths and other info.

## Detection Data Callback Example

@code{.cpp}
#include "LD2410Async.h"

HardwareSerial RadarSerial(1);
LD2410Async radar(RadarSerial);

void onDetection(LD2410Async* sender, bool presenceDetected) {
    if (presenceDetected) {
        Serial.println("Presence detected!");
    } else {
        Serial.println("No presence.");
    }
}

void setup() {
    Serial.begin(115200);
    RadarSerial.begin(256000, SERIAL_8N1, 32, 33); // RX=Pin 32, TX= Pin 33

    radar.begin();
    radar.onDetectionDataReceived(onDetection);
}

void loop() {
    // Nothing needed here - data is processed asynchronously.
}
@endcode

## Configuration Callbacks

Whenever the LD2410 sensor executes or reports configuration changes, the library provides two separate callback mechanisms to keep your application informed.

Use @ref LD2410Async::onConfigChanged "onConfigChanged()" to register a callback that is invoked whenever the sensor acknowledges and applies a configuration-changing command (for example, after setting sensitivities or timeouts). This event serves as a notification that the sensor has accepted a change, but it does not mean that the local configuration data has been refreshed. If you need updated values, you must explicitly request them from the sensor (e.g. with @ref LD2410Async::requestAllConfigSettingsAsync "requestAllConfigSettingsAsync()").

Use @ref LD2410Async::onConfigDataReceived "onConfigDataReceived()" to register a callback that is invoked whenever new configuration data has actually been received from the sensor. This happens after request commands such as @ref LD2410Async::requestAllConfigSettingsAsync "requestAllConfigSettingsAsync()" or @ref LD2410Async::requestGateParametersAsync "requestGateParametersAsync()". Within this callback, the library guarantees that the internal @ref LD2410Types::ConfigData "ConfigData" structure has been updated, so you can safely access it via @ref LD2410Async::getConfigData "getConfigData()" or @ref LD2410Async::getConfigDataRef "getConfigDataRef()".

@note 
Configuration data is **not sent automatically** by the sensor and is **not updated automatically** when internal changes occur. To refresh the local configuration structure, you must explicitly request the latest values from the sensor. The recommended way is to call @ref LD2410Async::requestAllConfigSettingsAsync "requestAllConfigSettingsAsync()", which retrieves the complete configuration in one operation.

## Configuration Callbacks Example

@code{.cpp}
// Define the callback function
void onConfigChanged(LD2410Async* sender) {
  Serial.println("Sensor acknowledged a configuration change.");

  // If you want the latest config values, explicitly request them
  sender->requestAllConfigSettingsAsync(onConfigReceived);
}

// Define a helper callback for when config data arrives
void onConfigReceived(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
  if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
    LD2410Types::ConfigData cfg = sender->getConfigData();
    Serial.print("No one timeout: ");
    Serial.println(cfg.noOneTimeout);
  }
}

// Somewhere in setup():
radar.onConfigChanged(onConfigChanged);
@endcode


## Async Commands Basics

The LD2410 sensor can be configured and queried using a large set of commands.

All commands that need to communicate with the sensor are implemented as asynchronous operations:
- They return immediately without blocking the main loop.
- Each command call itself returns a simple true or false:
    - true means the command was accepted for processing.
    - false means it could not be started (for example, because another command is already pending).
- When the sensor sends back an acknowledgement (ACK) or a response, the library automatically calls the user-provided callback with a result of type @ref LD2410Async::AsyncCommandResult "AsyncCommandResult". 
- Callbacks allow your application to handle SUCCESS, FAILED, TIMEOUT, or CANCELED in a clean and non-blocking way.

## List of Asynchronous Commands

- Config mode control
    - @ref LD2410Async::enableConfigModeAsync "enableConfigModeAsync()" - enable configuration mode.
    - @ref LD2410Async::disableConfigModeAsync "disableConfigModeAsync()" - disable configuration mode, return to normal detection.
- Engineering mode
    - @ref LD2410Async::enableEngineeringModeAsync "enableEngineeringModeAsync()" - enable detailed per-gate reporting.
    - @ref LD2410Async::disableEngineeringModeAsync "disableEngineeringModeAsync()" - disable engineering mode.
- Request configuration data
    - @ref LD2410Async::requestAllConfigSettingsAsync "requestAllConfigSettingsAsync()" - query all configuration parameters at once (bundles the next 3 commands into one). **Use this command whenever possible.**
    - @ref LD2410Async::requestGateParametersAsync "requestGateParametersAsync()" - query max gates, timeouts, sensitivities.
    - @ref LD2410Async::requestAuxControlSettingsAsync "requestAuxControlSettingsAsync()" - query auxiliary control settings.
    - @ref LD2410Async::requestDistanceResolutionAsync "requestDistanceResolutionAsync()" - query current resolution.
- Write configuration data
    - @ref LD2410Async::configureAllConfigSettingsAsync "configureAllConfigSettingsAsync()" - writes all config settings (combines the next 4 commands). Requires reboot if display resolution has changed. **Use this command whenever possible.**
    - @ref LD2410Async::configureMaxGateAndNoOneTimeoutAsync "configureMaxGateAndNoOneTimeoutAsync()" - set max gates and timeout.
    - @ref LD2410Async::configureDistanceGateSensitivityAsync "configureDistanceGateSensitivityAsync()" - set per-gate sensitivity (single or all).
    - @ref LD2410Async::configureDistanceResolutionAsync "configureDistanceResolutionAsync()" - set resolution (20 cm or 75 cm). Requires reboot.
    - @ref LD2410Async::configureAuxControlSettingsAsync "configureAuxControlSettingsAsync()" - configure light/output pins.
    - @ref LD2410Async::configureBaudRateAsync "configureBaudRateAsync()" - change UART baud rate. Requires reboot.
- Request static data    
    - @ref LD2410Async::requestAllStaticDataAsync "requestAllStaticDataAsync()" - query all static information (protocol version, buffer size, firmware version and bluetooth mac).
    - @ref LD2410Async::requestFirmwareAsync "requestFirmwareAsync()" - query firmware version.
- Bluetooth functions
    - @ref LD2410Async::enableBluetoothAsync "enableBluetoothAsync()" - turn on Bluetooth.
    - @ref LD2410Async::disableBluetoothAsync "disableBluetoothAsync()" - turn off Bluetooth.
    - @ref LD2410Async::requestBluetoothMacAddressAsync "requestBluetoothMacAddressAsync()" - query Bluetooth MAC address.
    - @ref LD2410Async::configureBluetoothPasswordAsync "configureBluetoothPasswordAsync()" - set custom password.
    - @ref LD2410Async::configureDefaultBluetoothPasswordAsync "configureDefaultBluetoothPasswordAsync()" - reset password to default.
- Auto-config
    - @ref LD2410Async::beginAutoConfigAsync "beginAutoConfigAsync()" - start auto-configuration routine.
    - @ref LD2410Async::requestAutoConfigStatusAsync "requestAutoConfigStatusAsync()" - query status of auto-config.
- Other
    - @ref LD2410Async::rebootAsync "rebootAsync()" - reboot the sensor (needed after some config changes).
    - @ref LD2410Async::restoreFactorySettingsAsync "restoreFactorySettingsAsync()" - restore default settings. Requires reboot.
- Async Command helpers
    - @ref LD2410Async::asyncIsBusy "asyncIsBusy()" - checks if another async command is pending
    - @ref LD2410Async::asyncCancel "asyncCancel()" - Cancels any currently pending async command. This will just abort the waiting for the Ack of the command. The actual command (or parts of it) have probably already been executed anyway.


## Async Command Example

@code{.cpp}
#include "LD2410Async.h"

HardwareSerial RadarSerial(1);
LD2410Async radar(RadarSerial);

// Callback method gets triggered when all config data has been received
void onConfigReceived(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
    if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
        // Access latest config via getConfigData()
        LD2410Types::ConfigData cfg = sender->getConfigData();
        Serial.print("Max moving gate: ");
        Serial.println(cfg.maxMotionDistanceGate);
        Serial.print("No one timeout: ");
        Serial.println(cfg.noOneTimeout);
    } else {
        Serial.println("Failed to retrieve config settings.");
    }
}

void setup() {
    Serial.begin(115200);
    RadarSerial.begin(256000, SERIAL_8N1, 32, 33); // RX=Pin 32, TX=Pin 33

    if (!radar.begin()) {
        Serial.println("Failed to initialize the LD2410Async library");
        return;
    }

    // Query all configuration parameters and register callback
    if (!radar.requestAllConfigSettingsAsync(onConfigReceived)) {
        Serial.println("Could not send config request (busy or failed).");
    }
}

void loop() {
    // Nothing required here – everything runs asynchronously
}

@endcode


## Best Practices for Async Commands

- **Check the return value** - all async methods return true if the command is getting executed or false when execution of the commands is not possible for some reason (e.g. another async command pending or a para is inavlid/out of range)
- **Check the result of the callback** - handle @ref LD2410Async::AsyncCommandResult "AsyncCommandResult" values such as `SUCCESS`, `FAILED`, `TIMEOUT`, or `CANCELED` as necessary.
- **Be aware of busy state** – before sending a command, ensure the library is not already executing another one.  
  Use @ref LD2410Async::asyncIsBusy "asyncIsBusy()" if needed. All async commands check the busy state internally and will return `false` if another command is already pending.  
- **Don’t block in callbacks** - keep callbacks short and non-blocking; offload heavy work to the main loop or a task.
- **Config mode handling** – you usually don’t need to manually enable or disable config mode; the library automatically handles this for commands that require it.  
  Only call @ref LD2410Async::enableConfigModeAsync "enableConfigModeAsync()" or @ref LD2410Async::disableConfigModeAsync "disableConfigModeAsync()" directly if you explicitly want to keep the sensor in config mode across multiple operations.  
- **Avoid overlapping commands** – sending a new command while another one is still pending can cause failures.  
  Always wait for the callback to complete before sending the next command, or call @ref LD2410Async::asyncCancel "asyncCancel()" if you need to abort the current operation.  

