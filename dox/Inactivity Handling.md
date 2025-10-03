# Inactivity Handling

The library has built in logic that will detect long inactivity of the sensor resp. long periods without any data coming in. The most common reason for a silent sensor is the conmfiguration mode, which has not been disabled.

If long silent periods are detected the lib will try to return its own state and the sensor back to normal operation mode. It will attempt the following steps (each separated by the timeout period for a async commands):
1. Cancel pending async commands, just in case the users code is still waiting for a callback.
2. If the cancel did not help, it will try to disable config mode, even if the lib thinks config mode is diabled.
3. As a last step, it will try to reboot the sensor.

If all those recovery steps dont work, the lib will try again to recover the sensor after the inactivity timeout period has elapsed again.

Inactivity handling can be enabled/disabled with the following commands:
- enableInactivityHandling()
- disableInactivityHandling()
- setInactivityHandling(true/false)

By default the inactivity handling is enabled

The timeout period for the inactivity handling can be set and read with:
- setInactivityTimeoutMs(timeoutInMilliseconds)
- getInactivityTimeoutMs()

The default timeout is 60000ms resp. 1 minute.
Make sure to set a value that is larger than the timeout for async commands (see getAsyncCommandTimeoutMs() and setAsyncCommandTimeoutMs()). Otherwise inactivity handling could kick in while commands are still pending (usually waiting for ack), 
which will result in the cancelation of the pending async command. To make sure this this will not happen, the inactivity handling will not use the set timeout, if it is shorter than the command time out and use the command timeout + 1000ms instead.