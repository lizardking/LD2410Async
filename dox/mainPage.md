@mainpage
# Introduction

 The LD2410 is a mmWave radar sensor capable of detecting both moving and
 stationary targets, reporting presence, distance, and per-gate signal strength.
 This class implements a non-blocking, asynchronous interface for communicating
 with the sensor over a UART stream.

 ## Features
 - Fully asynchronous operation with no blocking calls and callback.
 - Support for all native commands of the LD2410.
 - High level commands to allow for more consistent communication with the sensor.
 - Support for several instances if you have more than one sensor.   
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
@code{.cpp}
   HardwareSerial radarSerial(2);
   LD2410Async radar(radarSerial);

   void setup() {
     Serial.begin(115200);
     @ref LD2410Async::begin "radar.begin()" ;

     // Register callback for detection updates
     @ref LD2410Async::registerDetectionDataReceivedCallback "radar.registerDetectionDataReceivedCallback"([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       @ref LD2410Async::getDetectionDataRef "sender->getDetectionDataRef()".print();  // direct access, no copy
     });
   }

   void loop() {
     // Other application logic
   }
@endcode

## Examples
### Example: Using callback for presence detection updates
@code{.cpp}
     @ref LD2410Async::registerDetectionDataReceivedCallback "radar.registerDetectionDataReceivedCallback"([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       @ref LD2410Async::getDetectionDataRef "sender->getDetectionDataRef()".print();  // direct access, no copy
     });
@endcode

### Example: Access detection data without cloning
@code{.cpp}
   const DetectionData& data = @ref LD2410Async::getDetectionDataRef "radar.getDetectionDataRef()";  // no copy
   Serial.print("Target state: ");
   Serial.println(static_cast<int>(data.targetState));
@endcode

### Example: Clone config data, modify, and write back
@code{.cpp}
   ConfigData cfg = @ref LD2410Async::getConfigData "radar.getConfigData()";  // clone
   cfg.noOneTimeout = 60;
   @ref LD2410Async::configureAllConfigSettingsAsync "radar.configureAllConfigSettingsAsync"(cfg, false, [](LD2410Async* sender,
                                    AsyncCommandResult result,
                                    byte) {
     if (result == AsyncCommandResult::SUCCESS) {
       Serial.println("Config updated successfully!");
     }
   });
@endcode


