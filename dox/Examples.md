@page Examples Examples


The library ships with several example sketches that demonstrate typical usage as well as specialized test scenarios.  
All examples reside in the `examples/` folder of the library and are organized in subfolders named after the sketch.

@section Examples_Usage Usage Examples 

- @ref basicPresenceDetection_8ino  "basicPresenceDetection.ino"
  *Minimal example showing how to detect presence and print state changes via Serial.*  

- @ref receiveData_8ino  "receiveData.ino"
  *Demonstrates continuous reception of detection frames and printing all available sensor data.*  

- @ref changeConfig_8ino "changeConfig.ino" 
  *Shows how to request and modify configuration settings (e.g. timeouts, sensitivities) using async commands.*  

- @ref changeDistanceResolution_8ino "changeDistanceResolution.ino" 
  *Illustrates how to change the sensor’s distance resolution (20 cm vs 75 cm) and handle the required reboot.*  

- @ref simplePresenceDetectionWebservice_8ino  "implePresenceDetectionWebservice.ino"
  *Integrates presence detection with a simple web service endpoint, useful for IoT or home automation setups.*  

@section Examples_Test Test Sketches

- @ref enableConfigModeTest_8ino "enableConfigModeTest.ino" 
  *Test sketch to explicitly measure entering configuration mode and handling success/failure cases.
   The main use of this test is to check, whther enableConfigMode experiences timouts. *  

- @ref tortureTest_8ino  
  *Stress test that issues random async commands in a loop, tracking execution times and statistics.
  Used to check whther any errors accour over time.*  

- @ref unitTest_8ino "unitTest.ino" 
  *Collection of internal validation tests for library features, useful for regression testing during development.*  

