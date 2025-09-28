#pragma once
#include "Arduino.h"
#include "LD2410Async.h"  
#include "LD2410Defs.h"
#include "LD2410Types.h"

namespace LD2410CommandBuilder {

    inline bool buildMaxGateCommand(byte* out, byte maxMotionGate, byte maxStationaryGate, unsigned short noOneTimeout) {

        if (maxMotionGate < 2 || maxMotionGate > 8) return false;
        if (maxStationaryGate < 2 || maxStationaryGate > 8) return false;

        memcpy(out, LD2410Defs::maxGateCommandData, sizeof(LD2410Defs::maxGateCommandData));

        out[6] = maxMotionGate;
        out[12] = maxStationaryGate;

        memcpy(&out[18], &noOneTimeout, 2);

        return true;

    }

    inline bool buildGateSensitivityCommand(byte* out, byte gate, byte movingThreshold, byte stationaryThreshold) {

        if (gate > 8 && gate != 0xFF) return false; // 0–8 allowed, or 0xFF (all gates)

        memcpy(out, LD2410Defs::distanceGateSensitivityConfigCommandData,
            sizeof(LD2410Defs::distanceGateSensitivityConfigCommandData));

        if (gate > 8) {
            out[6] = 0xFF;
            out[7] = 0xFF;
        }
        else {
            out[6] = gate;
            out[7] = 0;
        }

        if (movingThreshold > 100) movingThreshold = 100;
        if (stationaryThreshold > 100) stationaryThreshold = 100;

        out[12] = movingThreshold;
        out[18] = stationaryThreshold;
        return true;
    }

    inline bool buildDistanceResolutionCommand(byte* out, LD2410Types::DistanceResolution resolution) {
        if (resolution == LD2410Types::DistanceResolution::RESOLUTION_75CM) {
            memcpy(out, LD2410Defs::setDistanceResolution75cmCommandData,
                sizeof(LD2410Defs::setDistanceResolution75cmCommandData));
        }
        else if (resolution == LD2410Types::DistanceResolution::RESOLUTION_20CM) {
            memcpy(out, LD2410Defs::setDistanceResolution20cmCommandData,
                sizeof(LD2410Defs::setDistanceResolution20cmCommandData));
        }
        else {
            return false;
        }
        return true;
    }

    inline bool buildAuxControlCommand(byte* out, LD2410Types::LightControl lightControl, byte lightThreshold, LD2410Types::OutputControl outputControl) {
        memcpy(out, LD2410Defs::setAuxControlSettingCommandData,
            sizeof(LD2410Defs::setAuxControlSettingCommandData));

        if (lightControl == LD2410Types::LightControl::NOT_SET) return false;
        if (outputControl == LD2410Types::OutputControl::NOT_SET) return false;

        out[4] = byte(lightControl);
        out[5] = lightThreshold;
        out[6] = byte(outputControl);
        return true;
    }

    inline bool buildBaudRateCommand(byte* out, byte baudRateSetting) {
        memcpy(out, LD2410Defs::setBaudRateCommandData, sizeof(LD2410Defs::setBaudRateCommandData));
        if (baudRateSetting < 1 || baudRateSetting > 8) return false;
        out[4] = baudRateSetting;
        return true;
    }

    inline bool buildBluetoothPasswordCommand(byte* out, const char* password) {
        if (!password) return false;
        size_t len = strlen(password);
        if (len > 6) return false;

        memcpy(out, LD2410Defs::setBluetoothPasswordCommandData,
            sizeof(LD2410Defs::setBluetoothPasswordCommandData));

        for (unsigned int i = 0; i < 6; i++) {
            if (i < strlen(password))
                out[4 + i] = byte(password[i]);
            else
                out[4 + i] = byte(' '); // pad with spaces
        }
        return true;
    }

}
