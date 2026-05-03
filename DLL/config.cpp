#include "shared.h"
#include "config.h"

const std::string configFilename = "SMT.ini";
ini::IniFile iniConfig;

namespace {
	std::atomic<bool> disableGameShifting = false;
	std::atomic<bool> skipNeutral = false;
	std::atomic<bool> requireClutch = false;
	std::atomic<bool> immersiveMode = false;
	std::atomic<bool> requireGearHeld = false;
	std::atomic<bool> debugLogging = false;

	const char* GetRuntimeOptionName(RuntimeOption option) {
		switch (option) {
		case RuntimeOption::DisableGameShifting:
			return "DISABLE GAME SHIFTING";
		case RuntimeOption::SkipNeutral:
			return "SKIP NEUTRAL";
		case RuntimeOption::RequireClutch:
			return "REQUIRE CLUTCH";
		case RuntimeOption::ImmersiveMode:
			return "IMMERSIVE MODE";
		case RuntimeOption::RequireGearHeld:
			return "REQUIRE GEAR HELD";
		case RuntimeOption::DebugLogging:
			return "DEBUG LOGGING";
		}
		return "";
	}

	std::atomic<bool>& GetRuntimeOptionStorage(RuntimeOption option) {
		switch (option) {
		case RuntimeOption::DisableGameShifting:
			return disableGameShifting;
		case RuntimeOption::SkipNeutral:
			return skipNeutral;
		case RuntimeOption::RequireClutch:
			return requireClutch;
		case RuntimeOption::ImmersiveMode:
			return immersiveMode;
		case RuntimeOption::RequireGearHeld:
			return requireGearHeld;
		case RuntimeOption::DebugLogging:
			return debugLogging;
		}
		return disableGameShifting;
	}

	bool TryParseRuntimeOption(const std::string& optionName, RuntimeOption& option) {
		if (optionName == "DISABLE GAME SHIFTING") {
			option = RuntimeOption::DisableGameShifting;
			return true;
		}
		if (optionName == "SKIP NEUTRAL") {
			option = RuntimeOption::SkipNeutral;
			return true;
		}
		if (optionName == "REQUIRE CLUTCH") {
			option = RuntimeOption::RequireClutch;
			return true;
		}
		if (optionName == "IMMERSIVE MODE") {
			option = RuntimeOption::ImmersiveMode;
			return true;
		}
		if (optionName == "REQUIRE GEAR HELD") {
			option = RuntimeOption::RequireGearHeld;
			return true;
		}
		if (optionName == "DEBUG LOGGING") {
			option = RuntimeOption::DebugLogging;
			return true;
		}
		return false;
	}

	bool IsTransientBindingValue(const std::string& value) {
		return value == "LISTENING" || value == "FOUND";
	}

	void WriteRuntimeOptionsToIni() {
		for (RuntimeOption option : {
			RuntimeOption::DisableGameShifting,
			RuntimeOption::SkipNeutral,
			RuntimeOption::RequireClutch,
			RuntimeOption::ImmersiveMode,
			RuntimeOption::RequireGearHeld,
			RuntimeOption::DebugLogging
			}) {
			iniConfig["OPTIONS"][GetRuntimeOptionName(option)] = GetRuntimeOption(option);
		}
	}
}

ini::IniFile WriteDefaultIniConfig() {
	ini::IniFile defaultIniConfig;
	defaultIniConfig["KEYBOARD"]["GEAR 1"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 2"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 3"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 4"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 5"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 6"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 7"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 8"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 9"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 10"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 11"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR 12"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR H"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR L-"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR L"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR L+"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR N"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR R"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR UP"] = "NONE";
	defaultIniConfig["KEYBOARD"]["GEAR DOWN"] = "NONE";
	defaultIniConfig["KEYBOARD"]["CLUTCH"] = "NONE";
	defaultIniConfig["KEYBOARD"]["RANGE HIGH"] = "NONE";
	defaultIniConfig["KEYBOARD"]["RANGE LOW"] = "NONE";
	defaultIniConfig["KEYBOARD"]["SHOW MENU"] = "Kb.211";

	defaultIniConfig["CONTROLLER"]["GEAR 1"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 2"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 3"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 4"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 5"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 6"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 7"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 8"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 9"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 10"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 11"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR 12"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR H"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR L-"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR L"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR L+"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR N"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR R"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR UP"] = "NONE";
	defaultIniConfig["CONTROLLER"]["GEAR DOWN"] = "NONE";
	defaultIniConfig["CONTROLLER"]["CLUTCH"] = "NONE";
	defaultIniConfig["CONTROLLER"]["RANGE HIGH"] = "NONE";
	defaultIniConfig["CONTROLLER"]["RANGE LOW"] = "NONE";
	defaultIniConfig["CONTROLLER"]["SHOW MENU"] = "NONE";

	defaultIniConfig["OPTIONS"]["DISABLE GAME SHIFTING"] = false;
	defaultIniConfig["OPTIONS"]["SKIP NEUTRAL"] = false;
	defaultIniConfig["OPTIONS"]["REQUIRE CLUTCH"] = false;
	defaultIniConfig["OPTIONS"]["IMMERSIVE MODE"] = false;
	defaultIniConfig["OPTIONS"]["REQUIRE GEAR HELD"] = false;
	defaultIniConfig["OPTIONS"]["DEBUG LOGGING"] = false;

	return defaultIniConfig;
}

void LoadIniConfig() {
	iniConfig = WriteDefaultIniConfig();
	std::ifstream is(configFilename);
	if (is.is_open()) {
		ini::IniFile tempConfig(configFilename);
		for (auto category : tempConfig) {
			for (auto entry : tempConfig[category.first]) {
				iniConfig[category.first][entry.first] = tempConfig[category.first][entry.first];
			}
		}
		LogMessage("Ini config found.");
	}
	else {
		iniConfig.save(configFilename);
		LogMessage("Ini config not found. Using default.");
	}
	SyncRuntimeOptionsFromIni();
}

void SaveIniConfig() {
	WriteRuntimeOptionsToIni();
	iniConfig.save(configFilename);
}

bool GetRuntimeOption(RuntimeOption option) {
	return GetRuntimeOptionStorage(option).load(std::memory_order_relaxed);
}

bool IsDebugLoggingEnabled() {
	return GetRuntimeOption(RuntimeOption::DebugLogging);
}

bool GetRuntimeOptionValue(const std::string& optionName) {
	RuntimeOption option;
	if (TryParseRuntimeOption(optionName, option)) {
		return GetRuntimeOption(option);
	}
	return iniConfig["OPTIONS"][optionName].as<bool>();
}

void SetBindingValue(const std::string& categoryName, const std::string& bindingName, const std::string& value) {
	const std::string currentValue = iniConfig[categoryName][bindingName].as<std::string>();
	if (currentValue == value) {
		return;
	}

	iniConfig[categoryName][bindingName] = value;
	if (!IsTransientBindingValue(value)) {
		SaveIniConfig();
	}
}

void SetRuntimeOptionValue(const std::string& optionName, bool value) {
	RuntimeOption option;
	if (TryParseRuntimeOption(optionName, option)) {
		GetRuntimeOptionStorage(option).store(value, std::memory_order_relaxed);
		if (option == RuntimeOption::DebugLogging) {
			LogMessage("Debug logging", value ? "enabled" : "disabled");
		}
		return;
	}
	iniConfig["OPTIONS"][optionName] = value;
}

void SyncRuntimeOptionsFromIni() {
	for (RuntimeOption option : {
		RuntimeOption::DisableGameShifting,
			RuntimeOption::SkipNeutral,
			RuntimeOption::RequireClutch,
			RuntimeOption::ImmersiveMode,
			RuntimeOption::RequireGearHeld,
			RuntimeOption::DebugLogging
			}) {
		GetRuntimeOptionStorage(option).store(
			iniConfig["OPTIONS"][GetRuntimeOptionName(option)].as<bool>(),
			std::memory_order_relaxed
		);
	}
}
