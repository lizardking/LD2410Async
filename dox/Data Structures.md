@page Data_Structures Data Structures
# LD2410 Data Structures

This section documents the main data structures provided by the library for accessing sensor state
and configuration:  
@ref LD2410Types::DetectionData "DetectionData" and @ref LD2410Types::ConfigData "ConfigData".  

---

## DetectionData

The @ref LD2410Types::DetectionData "DetectionData" struct holds the most recent detection data
reported by the radar. It is continuously updated as new frames arrive.  

You can access it through:  
- @ref LD2410Async::getDetectionData "getDetectionData()" – returns a copy  
- @ref LD2410Async::getDetectionDataRef "getDetectionDataRef()" – returns a const reference (efficient, no copy)

### Members

#### General
- **timestamp** (`unsigned long`)  
  Milliseconds since boot when this data was received.  

- **engineeringMode** (`bool`)  
  True if engineering mode data was received.  

#### Basic Detection Results
- **presenceDetected** (`bool`)  
  True if any target is detected.  

- **movingPresenceDetected** (`bool`)  
  True if a moving target is detected.  

- **stationaryPresenceDetected** (`bool`)  
  True if a stationary target is detected.  

- **targetState** (@ref LD2410Types::TargetState "TargetState")  
  Current detection state (no target, moving, stationary, both, auto config).  

- **movingTargetDistance** (`unsigned int`)  
  Distance in cm to the nearest moving target.  

- **movingTargetSignal** (`byte`)  
  Signal strength (0–100) of the moving target.  

- **stationaryTargetDistance** (`unsigned int`)  
  Distance in cm to the nearest stationary target.  

- **stationaryTargetSignal** (`byte`)  
  Signal strength (0–100) of the stationary target.  

- **detectedDistance** (`unsigned int`)  
  General detection distance in cm.  

#### Engineering Mode Data
(Only valid if **engineeringMode** is true.)

- **movingTargetGateSignalCount** (`byte`)  
  Number of gates with moving target signals.  

- **movingTargetGateSignals[9]** (`byte[9]`)  
  Per-gate signal strengths for moving targets.  

- **stationaryTargetGateSignalCount** (`byte`)  
  Number of gates with stationary target signals.  

- **stationaryTargetGateSignals[9]** (`byte[9]`)  
  Per-gate signal strengths for stationary targets.  

- **lightLevel** (`byte`)  
  Reported ambient light level (0–255).  

- **outPinStatus** (`bool`)  
  Current status of the OUT pin (true = high, false = low).  

---

## ConfigData

The @ref LD2410Types::ConfigData "ConfigData" struct stores the radar’s configuration parameters.  
This includes both static capabilities (like number of gates) and adjustable settings (like sensitivities and timeouts).  

Values are filled by commands such as:  
- @ref LD2410Async::requestAllConfigSettingsAsync "requestAllConfigSettingsAsync()"  
- @ref LD2410Async::requestGateParametersAsync "requestGateParametersAsync()"  
- @ref LD2410Async::requestAuxControlSettingsAsync "requestAuxControlSettingsAsync()"  
- @ref LD2410Async::requestDistanceResolutionAsync "requestDistanceResolutionAsync()"  

### Members

#### Radar Capabilities
- **numberOfGates** (`byte`)  
  Number of distance gates (2–8). Read-only; changing has no effect.  

#### Max Distance Gate Settings
- **maxMotionDistanceGate** (`byte`)  
  Furthest gate used for motion detection.  

- **maxStationaryDistanceGate** (`byte`)  
  Furthest gate used for stationary detection.  

#### Per-Gate Sensitivity
- **distanceGateMotionSensitivity[9]** (`byte[9]`)  
  Motion sensitivity values per gate (0–100).  

- **distanceGateStationarySensitivity[9]** (`byte[9]`)  
  Stationary sensitivity values per gate (0–100).  

#### Timeout
- **noOneTimeout** (`unsigned short`)  
  Timeout in seconds until “no presence” is declared.  

#### Distance Resolution
- **distanceResolution** (@ref LD2410Types::DistanceResolution "DistanceResolution")  
  Current distance resolution (20 cm or 75 cm).  
  Requires reboot to apply after change.  

#### Auxiliary Controls
- **lightThreshold** (`byte`)  
  Threshold for auxiliary light control (0–255).  

- **lightControl** (@ref LD2410Types::LightControl "LightControl")  
  Light-dependent auxiliary control mode.  

- **outputControl** (@ref LD2410Types::OutputControl "OutputControl")  
  Logic configuration of the OUT pin.  

---

## Summary

- **DetectionData** – continuously updated runtime measurements, including both simple presence and optional per-gate details when engineering mode is enabled.  
- **ConfigData** – sensor configuration and capabilities, only updated when explicitly requested.  

For efficiency, use the reference accessors (`getDetectionDataRef()`, `getConfigDataRef()`) whenever possible, and refresh `ConfigData` after writes to stay synchronized with the sensor.
