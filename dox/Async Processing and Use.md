# Async Processing & Commands

## Why asynchronous processing matters for the LD2410

The LD2410 radar sensor continuously streams detection frames and requires explicit configuration commands to adjust its behavior. 
If the library were implemented with blocking (synchronous) calls, every command would have to wait for the sensor’s response before the application could continue. 
Since enabling config mode or requesting data can take several seconds, blocking calls would freeze the main program loop and would disrupt other tasks.

An asynchronous approach avoids this problem. Commands are sent non-blocking, and the library reports back via callbacks when acknowledgements or data arrive. This has several benefits:
- Responsiveness – the main loop and other tasks keep running while waiting for the sensor.
- Efficiency – incoming data frames are parsed in the background without polling or busy-waiting.
- Scalability – multiple sensors or other asynchronous components (Wi-Fi, MQTT, UI updates) can run in parallel without interfering with each other.
- Robustness – timeouts and retries can be handled cleanly without stalling the system.

## Detection Data Callback

Whenever the LD2410 sensor transmits a valid data frame, the library automatically parses it and invokes the detection data callback. This allows your application to react immediately to presence changes without polling the sensor.

The callback delivers two things:
- A pointer to the LD2410Async instance that triggered the event. Allows for easy access to the members of that instance.
- A simple presenceDetected flag (true if presence has been detected, false otherwise).

This mechanism makes it easy to update your logic in real time — for example to turn lights on/off or send MQTT updates.

**Example**

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
    radar.registerDetectionDataReceivedCallback(onDetection);
}

void loop() {
    // Nothing needed here – data is processed asynchronously.
}
@endcode

## Async Commands

The LD2410 sensor can be configured and queried using a large set of commands.

All commands that need to coommunicate with the sensor are implemented as asynchronous operations:
- They return immediately without blocking the main loop.
- When the sensor sends back an acknowledgement (ACK) or a response, the library automatically calls the user-provided callback with the result.
- Callbacks allow your application to handle success, timeout, or failure in a clean and non-blocking way.

This design makes the library suitable for complex applications where other tasks (e.g. networking, user input) must continue running while waiting for the radar to respond.

### List of Asynchronous Commands

-Config mode control
    - enableConfigModeAsync() – enable configuration mode.
    - `disableConfigModeAsync()` – disable configuration mode, return to normal detection.
- Engineering mode
    - `enableEngineeringModeAsync()` – enable detailed per-gate reporting.
    - `disableEngineeringModeAsync()` – disable engineering mode.
- Request configuration data
    - `requestAllConfigSettingsAsync()` – query all configuration parameters at once (bundles the next 3 commands into one). **Use this command whenever possible.**
    - `requestGateParametersAsync()` – query max gates, timeouts, sensitivities.
    - `requestAuxControlSettingsAsync()` – query auxiliary control settings.
    - `requestDistanceResolutionAsync()` – query current resolution.
- Write configuration data
    - `configureAllConfigSettingsAsync()` - writes all config settings (combines the next 4 commands). Requires reboot if display resolution has changed. **Use this command whenever possible.**
    - `configureMaxGateAndNoOneTimeoutAsync()` – set max gates and timeout.
    - `configureDistanceGateSensitivityAsync()` – set per-gate sensitivity (single or all).
    - `configureDistanceResolutionAsync()` – set resolution (20 cm or 75 cm). Requires reboot.
    - `configureAuxControlSettingsAsync()` – configure light/output pins.
    - `configureBaudRateAsync()` – change UART baud rate. Requires reboot.
- Request static data    
    - `requestAllStaticDataAsync()` – query all static information (protocol version, buffer size, firmware version and bluetooth mac).
    - `requestFirmwareAsync()` – query firmware version.
- Bluetooth functions
    - `enableBluetoothAsync()` – turn on Bluetooth.
    - `disableBluetoothAsync()` – turn off Bluetooth.
    - `requestBluetoothMacAddressAsync()` – query Bluetooth MAC address.
    - `configureBluetoothPasswordAsync()` – set custom password.
    - `configureDefaultBluetoothPasswordAsync()` – reset password to default.
- Auto-config
    - `beginAutoConfigAsync()` – start auto-configuration routine.
    - `requestAutoConfigStatusAsync()` – query status of auto-config.
- Other
    - `rebootAsync()` – reboot the sensor (needed after some config changes).
    - `restoreFactorySettingsAsync()` – restore default settings. Requires reboot.
- Async Command helpers
    - `asyncIsBusy()` -  checks if another async command is pending
    - `asyncCancel()` - Cancels any currently pending async command. This will just abort the waiting for the Ack of the command. The actual command (or parts of it) have probably already been executed anyway.

**Example**

@code{.cpp}
#include "LD2410Async.h"

HardwareSerial RadarSerial(1);
LD2410Async radar(RadarSerial);

void onConfigReceived(LD2410Async* sender, LD2410Async::AsyncCommandResult result) {
    if (result == LD2410Async::AsyncCommandResult::SUCCESS) {
        // Access latest config via getConfigData()
        auto cfg = sender->getConfigData();
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

    radar.begin();

    // Query all configuration parameters
    radar.requestAllConfigSettingsAsync(onConfigReceived);
}

void loop() {
    // Async – nothing needed here
}
@endcode


## Best Practices for Async Commands

- **Be aware of busy state** – before sending a command, ensure the library is not already executing another one. Use asyncIsBusy() if needed. All commands are checking for busy state internally and will return false, if another command is pending.
- **Use callbacks** – never assume a command succeeded immediately. Handle SUCCESS, FAILED, TIMEOUT, or CANCELED in the callback.
- **Don’t block in callbacks** – keep callbacks short and non-blocking; offload heavy work to the main loop or a task.
- **Config mode handling** – you don’t need to manually enable/disable config mode; the library handles this automatically for commands that require it. Only use enableConfigModeAsync()/disableConfigModeAsync() directly if you need to stay in config mode across multiple operations.
- **Avoid overlapping commands** – sending a new command while one is pending will cause failures. Either wait for the callback or call asyncCancel() if you must interrupt.
