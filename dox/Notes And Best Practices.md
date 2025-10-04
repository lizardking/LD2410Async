@page BestPractices Important Notes and Best Practices

## Important Notes & Best Practices

When working with the LD2410Async library, keep the following points in mind:

- **Consider if you really need the lib**
  - The lib makes communication with the LD2410 easy and straightforward. However, for usages where you never need anything else than a simple presence detected signal, it might be easier more more efficent to just connect the out pin of the sensor to the microcontroller and not to connect the serial pins.
	This will allow you to query presence with a simple digital read command, without having to care about anything else.
	The downside of this is that you cant get more detailed detection data or do anything about the config of the sensor. This is where this lib comes in. 

- **Keep Callbacks Short**  
  - Callbacks are executed inside the radar’s processing task.  
  - Keep them **short and non-blocking** (e.g., update a variable or post to a queue).
  - Avoid long delay code, heavy computations, or blocking I/O inside callbacks - otherwise, sensor data processing may be delayed or datection data can get lost.

- **Async Command Busy State**  
  - The library ensures only one async command or sequence runs at a time.  
  - Check `asyncIsBusy()` before sending a new command.
  - Alway check the return value of the async commands. If false is returned the command has not been sent (either due to busy state or due to invalid paras). True means that the command has been sent and that the callback of the method will execute after completition of the command or after the timeout period.
  - If chaining commands, trigger the next command from the callback of the previous command. If the next command should only be executed, if the previous command was successfull, check the success para of the callback.

- **Config Mode Handling**  
  - The sensor must be in *config mode* to change settings.  
  - All methods/commands that talk to the sensor enable and disable config mode for you if necessary. This means that they will enable and disable the config mode if the config mode has not been active when the command is called. If the config mode is alredy active when calling the command, it will remain active after the command has completed resp. fires its callback. 
  - Activating the config mode is a time consuming operation (often more than 1.5 secs). Therefore sending a lot of commands can take quite a while. If a sequence with several commands has to be sent, it is a good idea to enable config mode first and deactivate it after the last command (make sure this also happends if commands fail/return false or dont report success in their callback).
  - Avoid leaving the sensor in config mode longer than necessary, since the sensor will not deliver any detection data while in config mode.

- **Engineering Mode**  
  - Engineering mode provides detailed gate signal data for development and debugging.  
  - Do **not** enable engineering mode in production unless required - it increases the amount of data sent and will trigger the detection callback more often.  
  - Leaving it on continuously can increase CPU load and reduce performance.

- **Presence Detection**  
  - Use the dedicated detection callback with the `presenceDetected` flag of the DetectionDataReceivedCallback (use onDetectionDataReceived() to register for that callback) if you only care about *whether* something is present.  
  - For advanced scenarios (distances, signals, engineering mode), you can access the full `DetectionData` via `getDetectionData()`.

- **Task Management**  
  - The library runs a FreeRTOS background task for receiving and processing data.  
  - You must use `begin()` to start it. If you ever need to stop it again, use `end()` to stop it gracefully.  
  - Do not block the main loop for long periods; the radar’s task will continue to run, but your application logic may lag.

- **Inactivity Handling**  
  - The library can automatically handle situation where the sensor doesnt send any data for a configurable period. Sice such situations are typically a result of the config mode being active accidentally, the lib will first try to disable the config mode and if that does not help it will try to reboot the sensor.  
  - You can enable or disable this behavior with `setInactivityHandling(bool enable)` depending on your use case.

- **Config Memory Wear**  
  - The LD2410 stores configuration in internal non-volatile memory.  
  - Repeatedly writing unnecessary config updates can wear out the memory over time.  
  - Only send config changes when values really need to be updated.