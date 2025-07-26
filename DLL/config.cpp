#include "shared.h"
#include "config.h"

const std::string configFilename = "SMT.ini";
ini::IniFile iniConfig;

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
	defaultIniConfig["KEYBOARD"]["SHOW MENU"] = "Kb.199";

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
}

void SaveIniConfig() {
	iniConfig.save(configFilename);
}