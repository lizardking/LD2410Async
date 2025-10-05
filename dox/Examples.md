@page Examples Examples

The library has several example sketches that demonstrate typical usage as well as specialized test scenarios.  
All examples reside in the `examples/` folder of the library.
 
@section Examples_Callback Example: Using callback for presence detection updates
 
 @code{.cpp}
     radar.onDetectionDataReceived([](LD2410Async* sender, bool presenceDetetced, byte userData) {
       sender->getDetectionDataRef().print();  // direct access, no copy
     });
 @endcode

 **Commands used in this example:**  
- @ref LD2410Async::onDetectionDataReceived "LD2410Async::onDetectionDataReceived()"  
- @ref LD2410Async::getDetectionDataRef "LD2410Async::getDetectionDataRef()"

 
@section Examples_Modify_Config Example: Clone config data, modify, and write back
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


@section Examples_Usage Usage Example Sketches 

- @ref basicPresenceDetection_8ino-example "basicPresenceDetection.ino"
  Minimal example showing how to detect presence and print state changes via Serial. 
 
- @ref receiveData_8ino-example "receiveData.ino"
  Demonstrates continuous reception of detection frames and printing all available sensor data. 
 
- @ref changeConfig_8ino-example "changeConfig.ino"
  Shows how to request and modify configuration settings (e.g. timeouts, sensitivities) using async commands.

- @ref changeDistanceResolution_8ino-example "changeDistanceResolution.ino"
  Illustrates how to change the sensor’s distance resolution (20 cm vs 75 cm) and handle the required reboot.

- - @ref simplePresenceDetectionWebservice_8ino-example "simplePresenceDetectionWebservice.ino"
  Integrates presence detection with a simple web service endpoint, useful as a starting point for IoT or home automation setups.

@section Examples_Test Test Sketches

- @ref enableConfigModeTest_8ino-example "enableConfigModeTest.ino"
  Test sketch to explicitly measure entering configuration mode and handling success/failure cases.
  The main use of this test is to check, whther enableConfigMode experiences timeouts.
 
- @ref tortureTest_8ino-example "tortureTest.ino"
  Stress test that issues random async commands in a loop, tracking execution times and statistics.
  Used to check whther any errors accour over time.
 
- @ref unitTest_8ino-example "unitTest.ino"
  Tests all methods of the lib. Useful for regression testing during development.

