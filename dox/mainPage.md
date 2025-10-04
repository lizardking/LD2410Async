@mainpage

## Introduction

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

## Documentation

The following pages provide detailed explanations of the libs components and usage patterns:

- @subpage Installation  
  * How to install the LD2410Async library via Arduino Library Manager or manually.  

- @subpage Async_Commands_And_Processing  
  * Covers the non-blocking design, callbacks for detection and configuration, and the full list of asynchronous commands supported by the LD2410.  

- @subpage Data_Structures  
  * Documents the core data structures: @ref LD2410Types::DetectionData "DetectionData" and @ref LD2410Types::ConfigData "ConfigData". Includes detailed descriptions of all members and their meanings.  

- @subpage Operation_Modes  
  * Explains the sensor’s Normal, Engineering, and Configuration modes, including how to switch between them and what data each provides.  

- @subpage Inactivity_Handling  
  * Describes the libs automatic recovery logic when the sensor becomes unresponsive, including how to configure and disable inactivity handling.  

- @subpage Examples
  * Lists all the examples in the lib.  

- @subpage BestPractices  
  * Important notes and best practices for using the library effectively (callback design, config handling, engineering mode, task management, and memory wear considerations).  

- @subpage Troubleshooting  
  * A guide to solving common issues such as no data received, callbacks not firing, async commands failing, or unexpected sensor reboots.


## Main Class Reference

All interaction with the sensor is performed through the main class:  
- @ref LD2410Async "LD2410Async"  
 

 ## Typical Usage

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
     radar.begin();

     // Register callback for detection updates
     radar.onDetectionDataReceived([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       sender->getDetectionDataRef().print();  // direct access, no copy
     });
   }

   void loop() {
     // Other application logic
   }
 @endcode

 **Commands used in this example:**  
- @ref LD2410Async::begin "LD2410Async::begin()"  
- @ref LD2410Async::onDetectionDataReceived "LD2410Async::onDetectionDataReceived()"  
- @ref LD2410Async::getDetectionDataRef "LD2410Async::getDetectionDataRef()"

 ## Examples

 ### Example Sketches
 
 Details on the example sketches in the examples folder can be found at the @ref Examples "Examples page".

 ### Example: Using callback for presence detection updates
 
 @code{.cpp}
     radar.onDetectionDataReceived([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       sender->getDetectionDataRef().print();  // direct access, no copy
     });
 @endcode

 **Commands used in this example:**  
- @ref LD2410Async::onDetectionDataReceived "LD2410Async::onDetectionDataReceived()"  
- @ref LD2410Async::getDetectionDataRef "LD2410Async::getDetectionDataRef()"

 ### Example: Clone config data, modify, and write back
 @code{.cpp}
   ConfigData cfg = radar.getConfigData();  // clone
   cfg.noOneTimeout = 60;
   radar.configureAllConfigSettingsAsync(cfg, false, [](LD2410Async* sender, AsyncCommandResult result, byte) {
     if (result == AsyncCommandResult::SUCCESS) {
       Serial.println("Config updated successfully!");
     }
   });
 @endcode


 **Commands used in this example:**  
- @ref LD2410Async::getConfigData "LD2410Async::getConfigData()"  
- @ref LD2410Async::configureAllConfigSettingsAsync "LD2410Async::configureAllConfigSettingsAsync()"


