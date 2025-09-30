@mainpage
# LD2410Async library for ESP32

## Introduction
 The LD2410 is a mmWave radar sensor capable of detecting both moving and
 stationary targets, reporting presence, distance, and per-gate signal strength.
 This class implements a non-blocking, asynchronous interface for communicating
 with the sensor over a UART stream (HardwareSerial, SoftwareSerial, etc.).

 ## Features
 - Fully asynchronous operation with no blocking calls and callback.
 - Support for all native commands of the LD2410.
 - High level commands to allow for more consistent communication with the sensor.
 - SUpport for several instances if you have more than one sensor.   
 - Continuous background task that parses incoming frames and updates data.
 - Automatic inactivity handling if sensor is stuck or unreposive (tries to disable config mode or reboot).

 ## Usage
 Typical workflow:
 1. Construct with a reference to a Stream object connected to the sensor.
 2. Call begin() to start the background task.
 3. Register callbacks for detection data and/or config updates.
 4. Use async commands to adjust sensor configuration as needed.
 5. Call end() to stop background processing if no longer required.

 Example:
 @code
   HardwareSerial radarSerial(2);
   LD2410Async radar(radarSerial);

   void setup() {
     Serial.begin(115200);
     radar.begin();

     // Register callback for detection updates
     radar.registerDetectionDataReceivedCallback([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       sender->getDetectionDataRef().print();  // direct access, no copy
     });
   }

   void loop() {
     // Other application logic
   }
 @endcode

 ## Examples
 ### Example: Using callback for presence detection updates
 @code
     radar.registerDetectionDataReceivedCallback([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       sender->getDetectionDataRef().print();  // direct access, no copy
     });
 @endcode

 ### Example: Access detection data without cloning
 @code
   const DetectionData& data = radar.getDetectionDataRef();  // no copy
   Serial.print("Target state: ");
   Serial.println(static_cast<int>(data.targetState));
 @endcode

 ### Example: Clone config data, modify, and write back
 @code
   ConfigData cfg = radar.getConfigData();  // clone
   cfg.noOneTimeout = 60;
   radar.configureAllConfigSettingsAsync(cfg, false, [](LD2410Async* sender,
                                    AsyncCommandResult result,
                                    byte) {
     if (result == AsyncCommandResult::SUCCESS) {
       Serial.println("Config updated successfully!");
     }
   });
 @endcode

 

