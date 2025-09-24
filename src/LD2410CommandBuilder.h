#pragma once
#include "Arduino.h"
#include "LD2410Async.h"  
#include "LD2410Defs.h"

namespace LD2410CommandBuilder {

    inline void buildMaxGateCommand(byte* out, byte maxMotionGate, byte maxStationaryGate, unsigned short noOneTimeout) {
        memcpy(out, LD2410Defs::maxGateCommandData, sizeof(LD2410Defs::maxGateCommandData));

        if (maxMotionGate > 8) maxMotionGate = 8;
        if (maxMotionGate < 2) maxMotionGate = 2;

        if (maxStationaryGate > 8) maxStationaryGate = 8;
        if (maxStationaryGate < 2) maxStationaryGate = 2;

        out[6] = maxMotionGate;
        out[12] = maxStationaryGate;

        memcpy(&out[18], &noOneTimeout, 4);
    }

    inline void buildGateSensitivityCommand(byte* out, byte gate, byte movingThreshold, byte stationaryThreshold) {
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
    }

    inline void buildDistanceResolutionCommand(byte* out, LD2410Async::DistanceResolution resolution) {
        if (resolution == LD2410Async::DistanceResolution::RESOLUTION_75CM) {
            memcpy(out, LD2410Defs::setDistanceResolution75cmCommandData,
                sizeof(LD2410Defs::setDistanceResolution75cmCommandData));
        }
        else if (resolution == LD2410Async::DistanceResolution::RESOLUTION_20CM) {
            memcpy(out, LD2410Defs::setDistanceResolution20cmCommandData,
                sizeof(LD2410Defs::setDistanceResolution20cmCommandData));
        }
    }

    inline void buildAuxControlCommand(byte* out, LD2410Async::LightControl lightControl, byte lightThreshold, LD2410Async::OutputControl outputControl) {
        memcpy(out, LD2410Defs::setAuxControlSettingCommandData,
            sizeof(LD2410Defs::setAuxControlSettingCommandData));

        out[4] = byte(lightControl);
        out[5] = lightThreshold;
        out[6] = byte(outputControl);
    }

    inline void buildBaudRateCommand(byte* out, byte baudRateSetting) {
        memcpy(out, LD2410Defs::setBaudRateCommandData, sizeof(LD2410Defs::setBaudRateCommandData));
        out[4] = baudRateSetting;
    }

    inline void buildBluetoothPasswordCommand(byte* out, const char* password) {
        memcpy(out, LD2410Defs::setBluetoothPasswordCommandData,
            sizeof(LD2410Defs::setBluetoothPasswordCommandData));

        for (unsigned int i = 0; i < 6; i++) {
            if (i < strlen(password))
                out[4 + i] = byte(password[i]);
            else
                out[4 + i] = byte(' '); // pad with spaces
        }
    }

}
