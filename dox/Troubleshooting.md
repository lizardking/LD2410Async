@page Troubleshooting Troubleshooting Guide

This page lists common issues you may encounter when using the **LD2410Async** library and provides steps to resolve them.

@section Troubleshooting_Problems Problem Scenarios

## No Data Received
- Verify wiring: ensure the LD2410 radar is connected to the correct UART pins.  
- Check the baud rate: the LD2410 defaults to **256000**.  
- Make sure you called @ref LD2410Async::begin "begin()" **after** initializing the serial port.  
- Ensure the sensor is not stuck in @ref Operation_Modes "Config Mode" - in config mode no detection data is sent.


## Callbacks Not Firing
- Confirm that you registered the callback before expecting data (e.g. with @ref LD2410Async::onDetectionDataReceived "onDetectionDataReceived()").  
- Ensure the sensor is not in config mode (@ref Operation_Modes).  
- If @ref Operation_Modes "Engineering Mode" is enabled, expect more frequent callbacks with additional data.

## Async Commands Not Working
- Only one async command can run at a time.  
  - Use @ref LD2410Async::asyncIsBusy "asyncIsBusy()" to check before sending another command.  
  - Always check the return value of async calls:  
    - **true** - command sent successfully (callback will fire).  
    - **false** - busy state or invalid parameters.  
- All commands require the sensor to be in config mode.  
  - The library enables/disables config mode automatically.  
  - If you manually enable it with @ref LD2410Async::enableConfigModeAsync "enableConfigModeAsync()", remember to disable it afterward with @ref LD2410Async::disableConfigModeAsync "disableConfigModeAsync()".

For more details see @ref Async_Commands_And_Processing.


## Unexpected Sensor Reboots
- Check if @ref Inactivity_Handling "inactivity handling" is enabled with @ref LD2410Async::setInactivityHandling "setInactivityHandling(true)".  
- If enabled, the library may automatically call @ref LD2410Async::rebootAsync "rebootAsync()" after long inactivity.

---

## Data Loss
- Keep callbacks **short and non-blocking**.  
  - Long or blocking callbacks may cause dropped frames.  
  - Best practice: copy the data in the callback and process later in your loop or another task.  
- Example: use @ref LD2410Async::onDetectionDataReceived "onDetectionDataReceived()" to capture `presenceDetected` quickly, then defer heavy work.  
- In rare cases, increasing the serial receive buffer size may help (though normally unnecessary).

See also @ref Async_Commands_And_Processing "Async Commands & Processing".

---

## Strange or Invalid Data
- Ensure the sensor is in **Normal Detection Mode** (see @ref Operation_Modes).  
- When in @ref Operation_Modes "Engineering Mode", the sensor produces additional raw data which may appear unusual if not parsed correctly.

---

@section Troubleshooting_Debugging Debugging

Enable debug output for troubleshooting communication issues. The following defines can be set in the LD2410Async.h file.

@code{.cpp}
#define LD2410ASYNC_DEBUG_LEVEL 1
#define LD2410ASYNC_DEBUG_DATA_LEVEL 2
#include <LD2410Async.h>
@endcode

Debug Levels:

- 0 - No debug (default).
- 1 - Basic debug messages.
- 2 - Include raw hex buffer dumps.

@warning
Do not enable debug in production:
Debugging adds overhead, negatively impacts performance, increases binary size, and will flood Serial output.