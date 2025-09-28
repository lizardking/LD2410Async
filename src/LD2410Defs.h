#pragma once
#include "Arduino.h"
#include "LD2410Async.h"   // for enums like LightControl, DistanceResolution

// LD2410Defs.h
// Internal protocol constants and command templates.
// Not part of the public API – subject to change.

namespace LD2410Defs
{
	constexpr size_t LD2410_Buffer_Size = 0x40;

	constexpr byte headData[4]{ 0xF4, 0xF3, 0xF2, 0xF1 };
	constexpr byte tailData[4]{ 0xF8, 0xF7, 0xF6, 0xF5 };
	constexpr byte headConfig[4]{ 0xFD, 0xFC, 0xFB, 0xFA };
	constexpr byte tailConfig[4]{ 4, 3, 2, 1 };

	constexpr byte configEnableCommand = 0xff;
	constexpr byte configEnableCommandData[6]{ 4, 0, configEnableCommand, 0, 1, 0 };

	constexpr byte configDisableCommand = 0xFE;
	constexpr byte configDisableCommandData[4]{ 2, 0, configDisableCommand, 0 };

	constexpr byte requestMacAddressCommand = 0xA5;
	constexpr byte requestMacAddressCommandData[6]{ 4, 0, requestMacAddressCommand, 0, 1, 0 };

	constexpr byte requestFirmwareCommand = 0xA0;
	constexpr byte requestFirmwareCommandData[4]{ 2, 0, requestFirmwareCommand, 0 };

	constexpr byte requestDistanceResolutionCommand = 0xAB;
	constexpr byte requestDistanceResolutionCommandData[4]{ 2, 0, requestDistanceResolutionCommand, 0 };

	constexpr byte setDistanceResolutionCommand = 0xAA;
	constexpr byte setDistanceResolution75cmCommandData[6]{ 4, 0, setDistanceResolutionCommand, 0, 0, 0 };
	constexpr byte setDistanceResolution20cmCommandData[6]{ 4, 0, setDistanceResolutionCommand, 0, 1, 0 };

	constexpr byte setBaudRateCommand = 0xA1;
	constexpr byte setBaudRateCommandData[6]{ 4, 0, setBaudRateCommand, 0, 7, 0 };

	constexpr byte restoreFactorySettingsAsyncCommand = 0xA2;
	constexpr byte restoreFactorSettingsCommandData[4]{ 2, 0, restoreFactorySettingsAsyncCommand, 0 };

	constexpr byte rebootCommand = 0xA3;
	constexpr byte rebootCommandData[4]{ 2, 0, rebootCommand, 0 };

	constexpr byte bluetoothSettingsCommand = 0xA4;
	constexpr byte bluetoothSettingsOnCommandData[6]{ 4, 0, bluetoothSettingsCommand, 0, 1, 0 };
	constexpr byte bluetoothSettingsOffCommandData[6]{ 4, 0, bluetoothSettingsCommand, 0, 0, 0 };

	constexpr byte getBluetoothPermissionsCommand = 0xA8;

	constexpr byte setBluetoothPasswordCommand = 0xA9;
	constexpr byte setBluetoothPasswordCommandData[10]{ 8, 0, setBluetoothPasswordCommand, 0, 0x48, 0x69, 0x4C, 0x69, 0x6E, 0x6B };

	constexpr byte requestParamsCommand = 0x61;
	constexpr byte requestParamsCommandData[4]{ 2, 0, requestParamsCommand, 0 };

	constexpr byte engineeringModeEnableComand = 0x62;
	constexpr byte engineeringModeEnableCommandData[4]{ 2, 0, engineeringModeEnableComand, 0 };

	constexpr byte engineeringModeDisableComand = 0x63;
	constexpr byte engineeringModeDisableCommandData[4]{ 2, 0, engineeringModeDisableComand, 0 };

	constexpr byte requestAuxControlSettingsCommand = 0xAE;
	constexpr byte requestAuxControlSettingsCommandData[4]{ 2, 0, requestAuxControlSettingsCommand, 0 };

	constexpr byte setAuxControlSettingsCommand = 0xAD;
	constexpr byte setAuxControlSettingCommandData[8]{ 6, 0, setAuxControlSettingsCommand, 0, 0, 0x80, 0, 0 };

	constexpr byte beginAutoConfigCommand = 0x0B;
	constexpr byte beginAutoConfigCommandData[6]{ 4, 0, beginAutoConfigCommand, 0, 0x0A, 0 };

	constexpr byte requestAutoConfigStatusCommand = 0x1B;
	constexpr byte requestAutoConfigStatusCommandData[4]{ 2, 0, requestAutoConfigStatusCommand, 0 };

	constexpr byte distanceGateSensitivityConfigCommand = 0x64;
	constexpr byte distanceGateSensitivityConfigCommandData[0x16]{
		0x14, 0, distanceGateSensitivityConfigCommand, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0
	};

	constexpr byte maxGateCommand = 0x60;
	constexpr byte maxGateCommandData[0x16]{
		0x14, 0, maxGateCommand, 0, 0, 0, 8, 0, 0, 0,
		1, 0, 8, 0, 0, 0, 2, 0, 5, 0, 0, 0
	};
}