#pragma once
#include "shared.h"

enum class RuntimeOption {
	DisableGameShifting,
	SkipNeutral,
	RequireClutch,
	ImmersiveMode,
	RequireGearHeld,
	DebugLogging
};

extern ini::IniFile iniConfig;
extern bool GetRuntimeOption(RuntimeOption option);
extern bool IsDebugLoggingEnabled();
extern bool GetRuntimeOptionValue(const std::string& optionName);
extern void SetBindingValue(const std::string& categoryName, const std::string& bindingName, const std::string& value);
extern void SetRuntimeOptionValue(const std::string& optionName, bool value);
extern void SyncRuntimeOptionsFromIni();
extern void LoadIniConfig();
extern void SaveIniConfig();
