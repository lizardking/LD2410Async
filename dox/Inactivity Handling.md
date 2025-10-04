@page Inactivity_Handling Inactivity Handling
# Inactivity Handling

## Basics
The library includes built-in logic to detect long inactivity of the sensor, i.e. long periods without any data being received.  
The most common cause for a "silent" sensor is that **configuration mode** was enabled but never disabled.

If long silent periods are detected, the library attempts to bring both its internal state and the sensor back to **normal operation mode**.  
Recovery is performed in the following steps (each step is separated by the current async command timeout period):  

1. Cancel any pending async commands, in case the user’s code is still waiting for a callback.  
   - @ref LD2410Async::asyncCancel "asyncCancel()"  
2. If canceling did not help, try to disable config mode — even if the library believes config mode is already disabled.  
   - @ref LD2410Async::disableConfigModeAsync "disableConfigModeAsync()"  
3. As a last step, attempt to reboot the sensor.  
   - @ref LD2410Async::rebootAsync "rebootAsync()"

If none of these recovery steps succeed, the library will retry after the inactivity timeout period has elapsed again.

## Enabling / Disabling Inactivity Handling

Inactivity handling can be enabled or disabled using:  

- @ref LD2410Async::enableInactivityHandling "enableInactivityHandling()"  
- @ref LD2410Async::disableInactivityHandling "disableInactivityHandling()"  
- @ref LD2410Async::setInactivityHandling "setInactivityHandling(bool enable)"  

By default, inactivity handling is **enabled**.

## Timeout Configuration

The inactivity timeout can be configured with:  

- @ref LD2410Async::setInactivityTimeoutMs "setInactivityTimeoutMs(timeoutInMilliseconds)"  
- @ref LD2410Async::getInactivityTimeoutMs "getInactivityTimeoutMs()"  

The default timeout is **60000 ms (1 minute)**.  
Setting the time out to 00 will disable inactivity handling.

**Important:**  
Always set a value larger than the timeout for async commands, which is controlled by:  
- @ref LD2410Async::setAsyncCommandTimeoutMs "setAsyncCommandTimeoutMs()"  
- @ref LD2410Async::getAsyncCommandTimeoutMs "getAsyncCommandTimeoutMs()"  

If the inactivity timeout is shorter than the async command timeout, inactivity handling could trigger while a command is still pending (usually waiting for an ACK). This would cause premature cancellation of the command.  

To avoid this, the library automatically adjusts the inactivity timeout:  
- If the configured inactivity timeout is shorter than the async command timeout, it will instead use **(async command timeout + 1000 ms)**.  
