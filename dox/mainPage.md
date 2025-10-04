@mainpage

## Introduction

 ![LD2410](/dox/gfx/LD2410.jpg)

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
  How to install the LD2410Async library via Arduino Library Manager or manually.  

- @subpage Async_Commands_And_Processing  
  Covers the non-blocking design, callbacks for detection and configuration, and the full list of asynchronous commands supported by the LD2410.  

- @subpage Data_Structures  
  Documents the core data structures: @ref LD2410Types::DetectionData "DetectionData" and @ref LD2410Types::ConfigData "ConfigData". Includes detailed descriptions of all members and their meanings.  

- @subpage Operation_Modes  
  Explains the sensor’s Normal, Engineering, and Configuration modes, including how to switch between them.  

- @subpage Inactivity_Handling  
  Describes the libs automatic recovery logic when the sensor becomes unresponsive, including how to configure and disable inactivity handling.  

- @subpage Examples
  Various examples that demonstrate and the the abilities of the lib.  

- @subpage BestPractices  
  Important notes and best practices for using the library effectively.  

- @subpage Troubleshooting  
  A guide to solving common issues such as no data received, callbacks not firing, async commands failing, or unexpected sensor reboots.


## Main Class Reference

All interaction with the sensor is performed through the main class:  
- @ref LD2410Async "LD2410Async"  
 

 ## Examples

 Details on the example sketches in the examples folder can be found at the @ref Examples "Examples page".

