@page Examples Examples


The library ships with several example sketches that demonstrate typical usage as well as specialized test scenarios.  
All examples reside in the `examples/` folder of the library and are organized in subfolders named after the sketch.

@section Examples_Usage Usage Examples **

- @ref basicPresenceDetection_8ino  
  *Minimal example showing how to detect presence and print state changes via Serial.*  

- @ref receiveData_8ino  
  *Demonstrates continuous reception of detection frames and printing all available sensor data.*  

- @ref changeConfig_8ino  
  *Shows how to request and modify configuration settings (e.g. timeouts, sensitivities) using async commands.*  

- @ref changeDistanceResolution_8ino  
  *Illustrates how to change the sensor’s distance resolution (20 cm vs 75 cm) and handle the required reboot.*  

- @ref simplePresenceDetectionWebservice_8ino  
  *Integrates presence detection with a simple web service endpoint, useful for IoT or home automation setups.*  

@section Examples_Test Test Sketches

- @ref enableConfigModeTest_8ino  
  *Test sketch to explicitly measure entering configuration mode and handling success/failure cases.*  

- @ref tortureTest_8ino  
  *Stress test that issues random async commands in a loop, tracking execution times and statistics.*  

- @ref unitTest_8ino  
  *Collection of internal validation tests for library features, useful for regression testing during development.*  

