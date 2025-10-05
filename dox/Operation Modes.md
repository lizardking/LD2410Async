@page Operation_Modes Operation Modes


The LD2410 sensor supports several operation modes, each with different behavior and purpose.

@section Operation_Modes_Normal_Mode Normal Detection Mode

In normal detection mode the sensor automatically transmits new detection data, usually several times per second.  

The transmitted data includes:
- Detection state  
- Distance (cm) to the nearest moving target  
- Signal strength (0-100) of the moving target  
- Distance (cm) to the nearest stationary target  
- Signal strength (0-100) of the stationary target  
- General detection distance (cm)  

Detection state is the most relevant. It can have the following values:
- 0 - No Target  
- 1 - Moving target  
- 2 - Stationary target  
- 3 - Moving and stationary target  
- 4 - Auto config in progress  
- 5 - Auto config success  
- 6 - Auto config failed  

Unless auto config has been explicitly triggered, only the values 0-3 occur.  

All detection data is represented in the @ref LD2410Types::DetectionData "DetectionData" struct, which also includes an `engineeringMode` flag that indicates whether engineering data fields in the struct are valid.

@section Operation_Modes_Engineering_Mode Engineering Mode

Engineering mode behaves similar to normal detection mode but provides additional data for detailed analysis:

- Number of gates with moving target signals  
- Per-gate signal strengths for moving targets  
- Number of gates with stationary target signals  
- Per-gate signal strengths for stationary targets  
- Reported ambient light level (0-255)  
- Current status of the OUT pin (true = high, false = low)  

This mode allows deeper inspection of what the sensor is detecting. The extended data is also part of the @ref LD2410Types::DetectionData "DetectionData" struct and can be distinguished using its `engineeringMode` flag.  

To control engineering mode:  
- @ref LD2410Async::enableEngineeringModeAsync "enableEngineeringModeAsync()"  
- @ref LD2410Async::disableEngineeringModeAsync "disableEngineeringModeAsync()"  

To check whether engineering mode is currently active:  
- @ref LD2410Async::isEngineeringModeEnabled "isEngineeringModeEnabled()"  

@section Operation_Modes_Configuration_Mode Configuration Mode

Configuration mode is required to send configuration commands to the sensor.  

- Before any configuration command can be executed, the sensors config mode must be enabled.  
- While config mode is active, the sensor will **not** send detection data. It remains silent except for acknowledgements (ACKs) in response to commands.  
- After finishing configuration, it is good practice to return the sensor as soon as possible.  

All commands in the @ref LD2410Async "LD2410Async" library automatically handle enabling and disabling config mode as needed.  

Because entering config mode can take significant time (120-3300 ms) and may occasionally fail and then requiring retries (handled internally), it can be more efficient to explicitly control config mode when sending multiple commands in sequence. In this case:  
- Enable config mode once with @ref LD2410Async::enableConfigModeAsync "enableConfigModeAsync()"  
- Send multiple commands (make sure to wait for the callbacks between the commands).  
- Disable config mode afterward with @ref LD2410Async::disableConfigModeAsync "disableConfigModeAsync()"  

To check whether config mode is currently active:  
- @ref LD2410Async::isConfigModeEnabled "isConfigModeEnabled()"  
