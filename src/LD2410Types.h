#pragma once


#include <Arduino.h>

/**
 * @ brief All enums, structs, and type helpers for LD2410Async
 */
namespace LD2410Types {
	/**
	 * @brief Represents the current target detection state reported by the radar.
	 *
	 * Values can be combined internally by the sensor depending on its
	 * signal evaluation logic. The AUTO-CONFIG states are special values
	 * that are only reported while the sensor is running its
	 * self-calibration routine.
	 * 
	 */

	enum class TargetState {
		NO_TARGET = 0,                  ///< No moving or stationary target detected.
		MOVING_TARGET = 1,              ///< A moving target has been detected.
		STATIONARY_TARGET = 2,          ///< A stationary target has been detected.
		MOVING_AND_STATIONARY_TARGET = 3, ///< Both moving and stationary targets detected.
		AUTOCONFIG_IN_PROGRESS = 4,     ///< Auto-configuration routine is running.
		AUTOCONFIG_SUCCESS = 5,         ///< Auto-configuration completed successfully.
		AUTOCONFIG_FAILED = 6           ///< Auto-configuration failed.
	};

	/**
	 * @brief Safely converts a numeric value to a TargetState enum.
	 *
	 * @param value Raw numeric value (0–6 expected).
	 *              - 0 → NO_TARGET
	 *              - 1 → MOVING_TARGET
	 *              - 2 → STATIONARY_TARGET
	 *              - 3 → MOVING_AND_STATIONARY_TARGET
	 *              - 4 → AUTOCONFIG_IN_PROGRESS
	 *              - 5 → AUTOCONFIG_SUCCESS
	 *              - 6 → AUTOCONFIG_FAILED
	 * @returns The matching TargetState value, or NO_TARGET if invalid.
	 * 
	 */
	static TargetState toTargetState(int value) {
		switch (value) {
		case 0: return TargetState::NO_TARGET;
		case 1: return TargetState::MOVING_TARGET;
		case 2: return TargetState::STATIONARY_TARGET;
		case 3: return TargetState::MOVING_AND_STATIONARY_TARGET;
		case 4: return TargetState::AUTOCONFIG_IN_PROGRESS;
		case 5: return TargetState::AUTOCONFIG_SUCCESS;
		case 6: return TargetState::AUTOCONFIG_FAILED;
		default: return TargetState::NO_TARGET; // safe fallback
		}
	}

	/**
	 * @brief Converts a TargetState enum value to a human-readable String.
	 *
	 * Useful for printing detection results in logs or Serial Monitor.
	 *
	 * @param state TargetState enum value.
	 * @returns Human-readable string such as "No target", "Moving target", etc.
	 *
	 */
	static String targetStateToString(TargetState state) {
		switch (state) {
		case TargetState::NO_TARGET: return "No target";
		case TargetState::MOVING_TARGET: return "Moving target";
		case TargetState::STATIONARY_TARGET: return "Stationary target";
		case TargetState::MOVING_AND_STATIONARY_TARGET: return "Moving and stationary target";
		case TargetState::AUTOCONFIG_IN_PROGRESS: return "Auto-config in progress";
		case TargetState::AUTOCONFIG_SUCCESS: return "Auto-config success";
		case TargetState::AUTOCONFIG_FAILED: return "Auto-config failed";
		default: return "Unknown";
		}
	}






	/**
	 * @brief Light-dependent control status of the auxiliary output.
	 *
	 * The radar sensor can control an external output based on ambient
	 * light level in combination with presence detection.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 *
	 */
	enum class LightControl {
		NOT_SET = -1,          ///< Placeholder resp. inital value. Do not use as a config value (will result in abortion of the config command).
		NO_LIGHT_CONTROL = 0,  ///< Output is not influenced by light levels.
		LIGHT_BELOW_THRESHOLD = 1, ///< Condition: light < threshold.
		LIGHT_ABOVE_THRESHOLD = 2  ///< Condition: light ≥ threshold.
	};
	/**
	 * @brief Safely converts a numeric value to a LightControl enum.
	 *
	 * @param value Raw numeric value (0–2 expected).
	 *              - 0 → NO_LIGHT_CONTROL
	 *              - 1 → LIGHT_BELOW_THRESHOLD
	 *              - 2 → LIGHT_ABOVE_THRESHOLD
	 * @returns The matching LightControl value, or NOT_SET if invalid.
	 *
	 */
	static LightControl toLightControl(int value) {
		switch (value) {
		case 0: return LightControl::NO_LIGHT_CONTROL;
		case 1: return LightControl::LIGHT_BELOW_THRESHOLD;
		case 2: return LightControl::LIGHT_ABOVE_THRESHOLD;
		default: return LightControl::NOT_SET;
		}
	}


	/**
	 * @brief Logic level behavior of the auxiliary output pin.
	 *
	 * Determines whether the output pin is active-high or active-low
	 * in relation to presence detection.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 *
	 */
	enum class OutputControl {
		NOT_SET = -1,                ///< Placeholder resp. inital value. Do not use as a config value (will result in abortion of the config command).
		DEFAULT_LOW_DETECTED_HIGH = 0, ///< Default low, goes HIGH when detection occurs.
		DEFAULT_HIGH_DETECTED_LOW = 1  ///< Default high, goes LOW when detection occurs.
	};

	/**
	 * @brief Safely converts a numeric value to an OutputControl enum.
	 *
	 * @param value Raw numeric value (0–1 expected).
	 *              - 0 → DEFAULT_LOW_DETECTED_HIGH
	 *              - 1 → DEFAULT_HIGH_DETECTED_LOW
	 * @returns The matching OutputControl value, or NOT_SET if invalid.
	 *
	 */
	static OutputControl toOutputControl(int value) {
		switch (value) {
		case 0: return OutputControl::DEFAULT_LOW_DETECTED_HIGH;
		case 1: return OutputControl::DEFAULT_HIGH_DETECTED_LOW;
		default: return OutputControl::NOT_SET;
		}
	}

	/**
	 * @brief State of the automatic threshold configuration routine.
	 *
	 * Auto-config adjusts the sensitivity thresholds for optimal detection
	 * in the current environment. This process may take several seconds.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 *
	 */
	enum class AutoConfigStatus {
		NOT_SET = -1,    ///< Status not yet retrieved.
		NOT_IN_PROGRESS, ///< Auto-configuration not running.
		IN_PROGRESS,     ///< Auto-configuration is currently running.
		COMPLETED        ///< Auto-configuration finished (success or failure).
	};

	/**
	 * @brief Safely converts a numeric value to an AutoConfigStatus enum.
	 *
	 * @param value Raw numeric value (0–2 expected).
	 *              - 0 → NOT_IN_PROGRESS
	 *              - 1 → IN_PROGRESS
	 *              - 2 → COMPLETED
	 * @returns The matching AutoConfigStatus value, or NOT_SET if invalid.
	 *
	 */
	static AutoConfigStatus toAutoConfigStatus(int value) {
		switch (value) {
		case 0: return AutoConfigStatus::NOT_IN_PROGRESS;
		case 1: return AutoConfigStatus::IN_PROGRESS;
		case 2: return AutoConfigStatus::COMPLETED;
		default: return AutoConfigStatus::NOT_SET;
		}
	}


	/**
	 * @brief Supported baud rates for the sensor’s UART interface.
	 *
	 * These values correspond to the configuration commands accepted
	 * by the LD2410. After changing the baud rate, a sensor reboot
	 * is required, and the ESP32’s UART must be reconfigured to match.
	 *
	 */
	enum class Baudrate {
		BAUDRATE_9600 = 1,  ///< 9600 baud.
		BAUDRATE_19200 = 2,  ///< 19200 baud.
		BAUDRATE_38400 = 3,  ///< 38400 baud.
		BAUDRATE_57600 = 4,  ///< 57600 baud.
		BAUDRATE_115200 = 5,  ///< 115200 baud.
		BAUDRATE_230500 = 6,  ///< 230400 baud.
		BAUDRATE_256000 = 7,  ///< 256000 baud (factory default).
		BAUDRATE_460800 = 8   ///< 460800 baud (high-speed).
	};

	/**
	 * @brief Distance resolution per gate for detection.
	 *
	 * The resolution defines how fine-grained the distance measurement is:
	 *   - RESOLUTION_75CM: Coarser, maximum range up to ~6 m, each gate ≈ 0.75 m wide.
	 *   - RESOLUTION_20CM: Finer, maximum range up to ~1.8 m, each gate ≈ 0.20 m wide.
	 *
	 * Use NOT_SET only as a placeholder; it is not a valid configuration value.
	 *
	 */
	enum class DistanceResolution {
		NOT_SET = -1,          ///< Placeholder resp. inital value. Do not use as a config value (will result in abortion of the config command).
		RESOLUTION_75CM = 0,   ///< Each gate ~0.75 m, max range ~6 m.
		RESOLUTION_20CM = 1    ///< Each gate ~0.20 m, max range ~1.8 m.
	};
	/**
	 * @brief Safely converts a numeric value to a DistanceResolution enum.
	 *
	 * @param value Raw numeric value (typically from a sensor response).
	 *              Expected values:
	 *                - 0 → RESOLUTION_75CM
	 *                - 1 → RESOLUTION_20CM
	 * @returns The matching DistanceResolution value, or NOT_SET if invalid.
	 *
	 */
	static DistanceResolution toDistanceResolution(int value) {
		switch (value) {
		case 0: return DistanceResolution::RESOLUTION_75CM;
		case 1: return DistanceResolution::RESOLUTION_20CM;
		default: return DistanceResolution::NOT_SET;
		}
	}

	/**
	  * @brief Holds the most recent detection data reported by the radar.
	  *
	  * This structure is continuously updated as new frames arrive.
	  * Values reflect either the basic presence information or, if
	  * engineering mode is enabled, per-gate signal details.
	 *
	  */
	struct DetectionData
	{
		unsigned long timestamp = 0; ///< Timestamp (ms since boot) when this data was received.

		// === Basic detection results ===

		bool engineeringMode = false; ///< True if engineering mode data was received.

		bool presenceDetected = false;           ///< True if any target is detected.
		bool movingPresenceDetected = false;     ///< True if a moving target is detected.
		bool stationaryPresenceDetected = false; ///< True if a stationary target is detected.

		TargetState targetState = TargetState::NO_TARGET; ///< Current detection state.
		unsigned int movingTargetDistance = 0; ///< Distance (cm) to the nearest moving target.
		byte movingTargetSignal = 0; ///< Signal strength (0–100) of the moving target.
		unsigned int stationaryTargetDistance = 0; ///< Distance (cm) to the nearest stationary target.
		byte stationaryTargetSignal = 0; ///< Signal strength (0–100) of the stationary target.
		unsigned int detectedDistance = 0; ///< General detection distance (cm).

		// === Engineering mode data ===
		byte movingTargetGateSignalCount = 0; ///< Number of gates with moving target signals.
		byte movingTargetGateSignals[9] = { 0 }; ///< Per-gate signal strengths for moving targets.

		byte stationaryTargetGateSignalCount = 0; ///< Number of gates with stationary target signals.
		byte stationaryTargetGateSignals[9] = { 0 }; ///< Per-gate signal strengths for stationary targets.

		byte lightLevel = 0; ///< Reported ambient light level (0–255).
		bool outPinStatus = 0; ///< Current status of the OUT pin (true = high, false = low).


		/**
		 * @brief Debug helper: print detection data contents to Serial.
		 */
		void print() const {
			Serial.println("=== DetectionData ===");

			Serial.print("  Timestamp: ");
			Serial.println(timestamp);

			Serial.print("  Engineering Mode: ");
			Serial.println(engineeringMode ? "Yes" : "No");

			Serial.print("  Target State: ");
			Serial.println(static_cast<int>(targetState));

			Serial.print("  Moving Target Distance (cm): ");
			Serial.println(movingTargetDistance);

			Serial.print("  Moving Target Signal: ");
			Serial.println(movingTargetSignal);

			Serial.print("  Stationary Target Distance (cm): ");
			Serial.println(stationaryTargetDistance);

			Serial.print("  Stationary Target Signal: ");
			Serial.println(stationaryTargetSignal);

			Serial.print("  Detected Distance (cm): ");
			Serial.println(detectedDistance);

			Serial.print("  Light Level: ");
			Serial.println(lightLevel);

			Serial.print("  OUT Pin Status: ");
			Serial.println(outPinStatus ? "High" : "Low");

			// --- Engineering mode fields ---
			if (engineeringMode) {
				Serial.println("  --- Engineering Mode Data ---");

				Serial.print("  Moving Target Gate Signal Count: ");
				Serial.println(movingTargetGateSignalCount);

				Serial.print("  Moving Target Gate Signals: ");
				for (int i = 0; i < movingTargetGateSignalCount; i++) {
					Serial.print(movingTargetGateSignals[i]);
					if (i < movingTargetGateSignalCount - 1) Serial.print(",");
				}
				Serial.println();

				Serial.print("  Stationary Target Gate Signal Count: ");
				Serial.println(stationaryTargetGateSignalCount);

				Serial.print("  Stationary Target Gate Signals: ");
				for (int i = 0; i < stationaryTargetGateSignalCount; i++) {
					Serial.print(stationaryTargetGateSignals[i]);
					if (i < stationaryTargetGateSignalCount - 1) Serial.print(",");
				}
				Serial.println();
			}

			Serial.println("======================");
		}

	};

	/**
	 * @brief Stores the sensor’s configuration parameters.
	 *
	 * This structure represents both static capabilities
	 * (e.g. number of gates) and configurable settings
	 * (e.g. sensitivities, timeouts, resolution).
	 *
	 * The values are typically filled by request commands
	 * such as requestAllConfigSettingsAsync() or requestGateParametersAsync() or
	 * requestAuxControlSettingsAsync() or requestDistanceResolutioncmAsync().
	 *
	 */
	struct ConfigData {
		// === Radar capabilities ===
		byte numberOfGates = 0; ///< Number of distance gates (2–8). This member is readonly resp. changing its value will not influence the radar setting when configureAllConfigSettingsAsync() is called. It is not 100% clear what this value stands for, but it seems to indicate the number of gates on the sensor.

		// === Max distance gate settings ===
		byte maxMotionDistanceGate = 0; ///< Furthest gate used for motion detection.
		byte maxStationaryDistanceGate = 0; ///< Furthest gate used for stationary detection.

		// === Per-gate sensitivity settings ===
		byte distanceGateMotionSensitivity[9] = { 0 }; ///< Motion sensitivity values per gate (0–100). 
		byte distanceGateStationarySensitivity[9] = { 0 }; ///< Stationary sensitivity values per gate (0–100).

		// === Timeout parameters ===
		unsigned short noOneTimeout = 0; ///< Timeout (seconds) until "no presence" is declared.

		// === Distance resolution ===
		DistanceResolution distanceResolution = DistanceResolution::NOT_SET; ///< Current distance resolution. A reboot is required to activate changed setting after calling configureAllConfigSettingsAsync() is called.

		// === Auxiliary controls ===
		byte lightThreshold = 0; ///< Threshold for auxiliary light control (0–255).
		LightControl lightControl = LightControl::NOT_SET; ///< Light-dependent auxiliary control mode.
		OutputControl outputControl = OutputControl::NOT_SET; ///< Logic configuration of the OUT pin.




		/**
		 * @brief Validates the configuration data for correctness.
		 *
		 * Ensures that enum values are set and values are within valid ranges.
		 * This method is called internally before applying a config
		 * via configureAllConfigSettingsAsync().
		 *
		 * @returns True if the configuration is valid, false otherwise.
		 */
		bool isValid() const {
			// Validate enum settings
			if (distanceResolution == DistanceResolution::NOT_SET) return false;
			if (lightControl == LightControl::NOT_SET) return false;
			if (outputControl == OutputControl::NOT_SET) return false;

			// Validate max distance gates
			if (maxMotionDistanceGate < 2 || maxMotionDistanceGate > numberOfGates) return false;
			if (maxStationaryDistanceGate < 1 || maxStationaryDistanceGate > numberOfGates) return false;

			// Validate sensitivities
			for (int i = 0; i < 9; i++) {
				if (distanceGateMotionSensitivity[i] > 100) return false;
				if (distanceGateStationarySensitivity[i] > 100) return false;
			}

			return true;
		}

		/**
		* @brief Compares this ConfigData with another for equality.
		*
		* @param other The other ConfigData instance to compare against.
		* @returns True if all fields are equal, false otherwise.
		*/
		bool equals(const ConfigData& other) const {
			if (numberOfGates != other.numberOfGates) return false;
			if (maxMotionDistanceGate != other.maxMotionDistanceGate) return false;
			if (maxStationaryDistanceGate != other.maxStationaryDistanceGate) return false;
			if (noOneTimeout != other.noOneTimeout) return false;
			if (distanceResolution != other.distanceResolution) return false;
			if (lightThreshold != other.lightThreshold) return false;
			if (lightControl != other.lightControl) return false;
			if (outputControl != other.outputControl) return false;

			for (int i = 0; i < 9; i++) {
				if (distanceGateMotionSensitivity[i] != other.distanceGateMotionSensitivity[i]) return false;
				if (distanceGateStationarySensitivity[i] != other.distanceGateStationarySensitivity[i]) return false;
			}
			return true;
		}

		/**
		 * @brief Debug helper: print configuration contents to Serial.
		 */
		void print() const {
			Serial.println("=== ConfigData ===");

			Serial.print("  Number of Gates: ");
			Serial.println(numberOfGates);

			Serial.print("  Max Motion Distance Gate: ");
			Serial.println(maxMotionDistanceGate);

			Serial.print("  Max Stationary Distance Gate: ");
			Serial.println(maxStationaryDistanceGate);

			Serial.print("  Motion Sensitivity: ");
			for (int i = 0; i < 9; i++) {
				Serial.print(distanceGateMotionSensitivity[i]);
				if (i < 8) Serial.print(",");
			}
			Serial.println();

			Serial.print("  Stationary Sensitivity: ");
			for (int i = 0; i < 9; i++) {
				Serial.print(distanceGateStationarySensitivity[i]);
				if (i < 8) Serial.print(",");
			}
			Serial.println();

			Serial.print("  No One Timeout: ");
			Serial.println(noOneTimeout);

			Serial.print("  Distance Resolution: ");
			Serial.println(static_cast<int>(distanceResolution));

			Serial.print("  Light Threshold: ");
			Serial.println(lightThreshold);

			Serial.print("  Light Control: ");
			Serial.println(static_cast<int>(lightControl));

			Serial.print("  Output Control: ");
			Serial.println(static_cast<int>(outputControl));

			Serial.println("===================");
		}
	};


}