#include "shared.h"
#include "input.h"
#include "config.h"
#include "gui.h"
#include "game_data.h"
#include "memory.h"

extern std::unordered_map <Vehicle*, std::atomic<bool>> IsInAuto;

OIS::InputManager* inputManager;
std::atomic<bool> keepAliveInput = true;
OIS::Keyboard* keyboard = nullptr;
std::vector<OIS::JoyStick*> joystickList;
std::unordered_map<std::string, bool> currentlyPressed;
std::set<std::string> tempPressed;
std::unordered_map<std::string, bool> wasPressedKb;
std::unordered_map<std::string, bool> wasPressedJoy;
std::atomic<int32_t> range = 0;

std::unordered_map<std::string, std::function<void()>> bindFunctions = {
	{ "GEAR 1",[]() { if (auto veh = GetCurrentVehicle()) { IsInAuto[veh] = true; veh->ShiftToGear(1); } }},
	{ "GEAR 2",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(2); } },
	{ "GEAR 3",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(3); } },
	{ "GEAR 4",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(4); } },
	{ "GEAR 5",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(5); } },
	{ "GEAR 6",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(6); } },
	{ "GEAR 7",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(7); } },
	{ "GEAR 8",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(8); } },
	{ "GEAR 9",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(9); } },
	{ "GEAR 10",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(10); } },
	{ "GEAR 11",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(11); } },
	{ "GEAR 12",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(12); } },
	{ "GEAR H",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToHighGear(); }},
	{ "GEAR L-",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToLowMinusGear(); }},
	{ "GEAR L",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToLowGear(); }},
	{ "GEAR L+",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToLowPlusGear(); }},
	{ "GEAR N",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToGear(0); }},
	{ "GEAR R",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToReverseGear(); }},
	{ "GEAR UP",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToNextGear(); }},
	{ "GEAR DOWN",[]() { if (auto veh = GetCurrentVehicle()) veh->ShiftToPrevGear(); }},
	{ "CLUTCH",[]() { return; } },
	{ "RANGE HIGH",[]() { if (range < 1) range++; }},
	{ "RANGE LOW",[]() { if (range > -1) range--; }},
	{ "SHOW MENU",[]() {showGui = !showGui; } }
};

extern void DetachDLL();

std::string abbreviate(const std::string& input) {
	std::stringstream ss(input);
	std::string word;
	std::string abbreviation;

	while (ss >> word) {
		if (!word.empty()) {
			abbreviation += toupper(word[0]);
		}
	}

	return abbreviation;
}

namespace SMT {

	bool KeyListener::keyPressed(const OIS::KeyEvent& e) {
		std::string entry = "Kb." + std::to_string(e.key);
		if (!currentlyPressed[entry]) {
			tempPressed.emplace(entry);
		}
		currentlyPressed[entry] = true;
		return true;
	}

	bool KeyListener::keyReleased(const OIS::KeyEvent& e) {
		currentlyPressed["Kb." + std::to_string(e.key)] = false;
		return true;
	}

	bool JoyStickListener::buttonPressed(const OIS::JoyStickEvent& e, int button) {
		std::string entry = abbreviate(e.device->vendor()) + ".b." + std::to_string(button);
		if (!currentlyPressed[entry]) {
			tempPressed.emplace(entry);
		}
		currentlyPressed[entry] = true;
		return true;
	}

	bool JoyStickListener::buttonReleased(const OIS::JoyStickEvent& e, int button) {
		currentlyPressed[abbreviate(e.device->vendor()) + ".b." + std::to_string(button)] = false;
		return true;
	}

	bool JoyStickListener::axisMoved(const OIS::JoyStickEvent& e, int axis) {
		if (e.state.mAxes[axis].abs > 20000) {
			std::string entry = abbreviate(e.device->vendor()) + ".a." + std::to_string(axis) + ".p";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".a." + std::to_string(axis) + ".p"] = false;
		}
		if (e.state.mAxes[axis].abs < -20000) {
			std::string entry = abbreviate(e.device->vendor()) + ".a." + std::to_string(axis) + ".n";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".a." + std::to_string(axis) + ".n"] = false;
		}
		return true;
	}

	bool JoyStickListener::sliderMoved(const OIS::JoyStickEvent& e, int sliderID) {
		if (e.state.mSliders[sliderID].abX > 20000) {
			std::string entry = abbreviate(e.device->vendor()) + ".s.x." + std::to_string(sliderID) + ".p";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".s.x." + std::to_string(sliderID) + ".p"] = false;
		}
		if (e.state.mSliders[sliderID].abX < -20000) {
			std::string entry = abbreviate(e.device->vendor()) + ".s.x." + std::to_string(sliderID) + ".n";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".s.x." + std::to_string(sliderID) + ".n"] = false;
		}
		if (e.state.mSliders[sliderID].abY > 20000) {
			std::string entry = abbreviate(e.device->vendor()) + ".s.y." + std::to_string(sliderID) + ".p";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".s.y." + std::to_string(sliderID) + ".p"] = false;
		}
		if (e.state.mSliders[sliderID].abY < -20000) {
			std::string entry = abbreviate(e.device->vendor()) + ".s.y." + std::to_string(sliderID) + ".n";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".s.y." + std::to_string(sliderID) + ".n"] = false;
		}
		return true;
	}

	bool JoyStickListener::povMoved(const OIS::JoyStickEvent& e, int pov) {
		if ((e.state.mPOV[pov].direction & OIS::Pov::North) != 0) {
			std::string entry = abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".up";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".up"] = false;
		}

		if ((e.state.mPOV[pov].direction & OIS::Pov::South) != 0) {
			std::string entry = abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".down";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".down"] = false;
		}

		if ((e.state.mPOV[pov].direction & OIS::Pov::East) != 0) {
			std::string entry = abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".right";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".right"] = false;
		}

		if ((e.state.mPOV[pov].direction & OIS::Pov::West) != 0) {
			std::string entry = abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".left";
			if (!currentlyPressed[entry]) {
				tempPressed.emplace(entry);
			}
			currentlyPressed[entry] = true;
		}
		else {
			currentlyPressed[abbreviate(e.device->vendor()) + ".p." + std::to_string(pov) + ".left"] = false;
		}
		return true;
	}
}

DWORD WINAPI ProcessInput(LPVOID lpReserved) {
	LogMessage("Processing input");
	while (keepAliveInput) {
		auto nextFrameTime = std::chrono::steady_clock::now();
		if (GetForegroundWindow() == window) {
			std::set<std::string> functionsToRun;
			int32_t keyCount = 0;
			keyboard->capture();
			if (GetForegroundWindow() == window) {
				for (auto& js : joystickList) {
					js->capture();
				}
			}
			bool goToNeutral = iniConfig["OPTIONS"]["REQUIRE GEAR HELD"].as<bool>();
			for (auto action : iniConfig["KEYBOARD"]) {
				bool pressed = true;
				if (action.second.as<std::string>() == "FOUND") {
					if (tempPressed.size() > 0) {
						std::string tempStr = "";
						for (auto key : tempPressed) {
							tempStr += key;
							tempStr += "+";
						}
						tempStr.pop_back();
						iniConfig["KEYBOARD"][action.first] = tempStr;
					}
					else {
						iniConfig["KEYBOARD"][action.first] = "NONE";
					}
					tempPressed.clear();
				}
				else {
					int32_t cnt = 0;
					for (auto part : action.second.as<std::string>() | std::views::split('+')) {
						cnt++;
						if (!currentlyPressed[std::string(part.begin(), part.end())]) {
							if (action.first.starts_with("GEAR") && action.second.as<std::string>() != "NONE") {
								if ((std::string(part.begin(), part.end()) == iniConfig["KEYBOARD"]["RANGE HIGH"].as<std::string>() && range == 1) ||
									(std::string(part.begin(), part.end()) == iniConfig["KEYBOARD"]["RANGE LOW"].as<std::string>() && range == -1)) {
									continue;
								}
							}
							pressed = false;
							break;
						}
					}
					if (pressed && action.first.starts_with("GEAR")) {
						goToNeutral = false;
					}
					if (pressed && wasPressedKb[action.first] == false) {
						if (cnt > keyCount) { functionsToRun.clear(); }
						functionsToRun.emplace(action.first);
					}
				}
				wasPressedKb[action.first] = pressed;
			}
			for (auto action : iniConfig["CONTROLLER"]) {
				bool pressed = true;
				if (action.second.as<std::string>() == "FOUND") {
					if (tempPressed.size() > 0) {
						std::string tempStr = "";
						for (auto key : tempPressed) {
							tempStr += key;
							tempStr += "+";
						}
						tempStr.pop_back();
						iniConfig["CONTROLLER"][action.first] = tempStr;
					}
					else {
						iniConfig["CONTROLLER"][action.first] = "NONE";
					}
					tempPressed.clear();
				}
				else {
					int32_t cnt = 0;
					for (auto part : action.second.as<std::string>() | std::views::split('+')) {
						cnt++;
						if (!currentlyPressed[std::string(part.begin(), part.end())]) {
							if (action.first.starts_with("GEAR") && action.second.as<std::string>() != "NONE") {
								if ((std::string(part.begin(), part.end()) == iniConfig["CONTROLLER"]["RANGE HIGH"].as<std::string>() && range == 1) ||
									(std::string(part.begin(), part.end()) == iniConfig["CONTROLLER"]["RANGE LOW"].as<std::string>() && range == -1)) {
									continue;
								}
							}
							pressed = false;
							break;
						}
					}
					if (pressed && action.first.starts_with("GEAR")) {
						goToNeutral = false;
					}
					if (pressed && wasPressedJoy[action.first] == false) {
						if (cnt > keyCount) { functionsToRun.clear(); }
						functionsToRun.emplace(action.first);
					}
				}
				wasPressedJoy[action.first] = pressed;
			}
			for (auto fnc : functionsToRun) {
				bindFunctions[fnc]();
				if (iniConfig["OPTIONS"]["REQUIRE CLUTCH"].as<bool>()) {
					if (auto veh = GetCurrentVehicle()) {
						if (!wasPressedKb["CLUTCH"] && !wasPressedJoy["CLUTCH"]) {
							if (fnc.starts_with("GEAR") && fnc != "GEAR N") {
								veh->StallCounter = 5;
							}
						}
					}
				}
			}
			if (auto veh = GetCurrentVehicle()) {
				if (goToNeutral && veh->TruckAction->Gear_1 != 0) {
					bindFunctions["GEAR N"]();
				}
			}
		}

		if (GetAsyncKeyState(VK_END) & 0x8000 && GetAsyncKeyState(VK_LCONTROL) & 0x8000 && GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
			DetachDLL();
		}

		nextFrameTime += std::chrono::milliseconds(16);
		std::this_thread::sleep_until(nextFrameTime);
	}
	return TRUE;
}

void InitInput() {
	CoInitialize(nullptr);
	OIS::ParamList paramlist;
	std::ostringstream windowHWNDStr;
	windowHWNDStr << (size_t)window;
	paramlist.insert(std::make_pair(std::string("WINDOW"), windowHWNDStr.str()));
	inputManager = OIS::InputManager::createInputSystem(paramlist);

	keyboard = static_cast<OIS::Keyboard*>(inputManager->createInputObject(OIS::OISKeyboard, true));
	const OIS::DeviceList& deviceList = inputManager->listFreeDevices();
	for (auto& device : deviceList) {
		if (device.first == OIS::OISJoyStick) {
			joystickList.push_back(static_cast<OIS::JoyStick*>(inputManager->createInputObject(device.first, true)));
		}
	}

	SMT::KeyListener* myKeyListener = new SMT::KeyListener();
	keyboard->setEventCallback(myKeyListener);
	SMT::JoyStickListener* myJoyStickListener = new SMT::JoyStickListener();
	for (auto& js : joystickList) {
		js->setEventCallback(myJoyStickListener);
	}
	CreateThread(nullptr, 0, ProcessInput, GetModuleHandleA(NULL), 0, nullptr);
}

void ShutdownInput() {
	keepAliveInput = false;
	Sleep(1000);
	if (inputManager) {
		if (keyboard) {
			inputManager->destroyInputObject(keyboard);
		}
		for (auto& js : joystickList) {
			inputManager->destroyInputObject(js);
		}
		OIS::InputManager::destroyInputSystem(inputManager);
	}
}