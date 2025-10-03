
# LD2410 Operation Modes

The LD2410 has several operation modes. Normal detection mode, engginnering/enhance detection mode and configuration mode. In each mode the sensor behaves differently.

## Normal Detection Mode

In normal detection mode the sensor automatically send new detection data, usually several times per second. 

The sent data includes:
- Detection state.
- Distance (cm) to the nearest moving target.
- Signal strength (0–100) of the moving target.
- Distance (cm) to the nearest stationary target.
- Signal strength (0–100) of the stationary target.
- General detection distance (cm).

Detection state is the most relevant. It has one of the following values:
- 0 → No Target
- 1 → Moving target
- 2 → Stationary target
- 3 → Moving and stationary target
- 4 → Auto config in progress
- 5 → Auto config success
- 6 → Auto config failed

Unless auto config has been explicitly triggered, only the values 0-3 occur.

## Engineering Mode

Behaves similar to the normal mode but send addition data:
- Number of gates with moving target signals.
- Per-gate signal strengths for moving targets.
- Number of gates with stationary target signals.
- Per-gate signal strengths for stationary targets.
- Reported ambient light level (0–255).
- Current status of the OUT pin (true = high, false = low).

This allows for more detailed analysis of what the sensor is detecting.

## Configuration Mode

Config mode is used to send commands to the sensor. Before any command, e.g. to configure a setting, can be sent to the sensor, config mode has to be enabled using the config mode enable command (the only command allowed in the other modes).

While config mode is active, the sensor will not send any detection data and remain silent, appart from send acknowledgements/acks for the commands it receives. For this reason it is good practice, to disable config mode again, when all necessary cammnds have been sent.

All commands in the LD2410Async lib are implemented to automatically enable and disable to config mode.

Since the enable config mode command can take a long time to execute (varies anywhere between 120ms - 3300ms) and sometimes even failes and needs a retry (also automatically handled in the lib), it is also possible to enabled and disable the config mode with specific commands. This allows to sends several commands in a row without repeatedly enabling/disabling config mode and wasting a lot of time.
All other commands will detect that config mode is enabled when they are executed and will keep it enabled when done. So dont forget to disable config mode in the code when all comands are sent.
