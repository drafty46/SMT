#include "shared.h"
#include "input.h"
#include "config.h"
#include "gui.h"
#include "game_data.h"

extern std::unordered_map <Vehicle*, std::atomic<bool>> IsInAuto;

OIS::InputManager* inputManager;
std::atomic<bool> keepAlive = true;
OIS::Keyboard* keyboard = nullptr;
std::vector<OIS::JoyStick*> joystickList;
std::unordered_map<std::string, bool> currentlyPressed;
std::vector<std::string> tempPressed;
std::unordered_map<std::string, bool> wasPressedKb;
std::unordered_map<std::string, bool> wasPressedJoy;

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
	{ "GEAR UP",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToNextGear(); }},
	{ "GEAR DOWN",[]() {if (auto veh = GetCurrentVehicle()) veh->ShiftToPrevGear(); }},
	{ "CLUTCH",[]() { return; } },
	{ "SHOW MENU",[]() {showGui = !showGui; } }
};

extern void DetachDLL();

namespace SMT {

	bool KeyListener::keyPressed(const OIS::KeyEvent& e) {
		currentlyPressed[e.device->vendor() + ".k." + std::to_string(e.key)] = true;
		return true;
	}

	bool KeyListener::keyReleased(const OIS::KeyEvent& e) {
		currentlyPressed[e.device->vendor() + ".k." + std::to_string(e.key)] = false;
		return true;
	}

	bool JoyStickListener::buttonPressed(const OIS::JoyStickEvent& e, int button) {
		currentlyPressed[e.device->vendor() + ".b." + std::to_string(button)] = true;
		return true;
	}

	bool JoyStickListener::buttonReleased(const OIS::JoyStickEvent& e, int button) {
		currentlyPressed[e.device->vendor() + ".b." + std::to_string(button)] = false;
		return true;
	}

	bool JoyStickListener::axisMoved(const OIS::JoyStickEvent& e, int axis) {
		if (e.state.mAxes[axis].abs > 25000) {
			currentlyPressed[e.device->vendor() + ".a." + std::to_string(axis) + ".p"] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".a." + std::to_string(axis) + ".p"] = false;
		}
		if (e.state.mAxes[axis].abs < -25000) {
			currentlyPressed[e.device->vendor() + ".a." + std::to_string(axis) + ".n"] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".a." + std::to_string(axis) + ".n"] = false;
		}
		return true;
	}

	bool JoyStickListener::sliderMoved(const OIS::JoyStickEvent& e, int sliderID) {
		if (e.state.mSliders[sliderID].abX) {
			currentlyPressed[e.device->vendor() + ".s." + std::to_string(sliderID)] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".s." + std::to_string(sliderID)] = false;
		}
		return true;
	}

	bool JoyStickListener::povMoved(const OIS::JoyStickEvent& e, int pov) {
		if ((e.state.mPOV[pov].direction & OIS::Pov::North) != 0) {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".up"] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".up"] = false;
		}

		if ((e.state.mPOV[pov].direction & OIS::Pov::South) != 0) {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".down"] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".down"] = false;
		}

		if ((e.state.mPOV[pov].direction & OIS::Pov::East) != 0) {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".right"] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".right"] = false;
		}

		if ((e.state.mPOV[pov].direction & OIS::Pov::West) != 0) {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".left"] = true;
		}
		else {
			currentlyPressed[e.device->vendor() + ".p." + std::to_string(pov) + ".left"] = false;
		}
		return true;
	}
}

DWORD WINAPI ProcessInput(LPVOID lpReserved) {
	LogMessage("Processing input");
	do {
		auto nextFrameTime = std::chrono::steady_clock::now();
		std::set<std::string> functionsToRun;
		int32_t keyCount = 0;
		keyboard->capture();
		if (GetForegroundWindow() == window) {
			for (auto& js : joystickList) {
				js->capture();
			}
		}
		for (auto action : iniConfig["KEYBOARD"]) {
			bool pressed = true;
			if (action.second.as<std::string>() == "LISTENING") {
				for (auto key : currentlyPressed) {
					if (key.second && std::find(tempPressed.begin(), tempPressed.end(), key.first) == tempPressed.end()) {
						tempPressed.push_back(key.first);
					}
				}
			}
			else if (action.second.as<std::string>() == "FOUND") {
				if (tempPressed.size() > 0) {
					std::string tempStr = "";
					std::sort(tempPressed.begin(), tempPressed.end());
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
						pressed = false;
						break;
					}
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
			if (action.second.as<std::string>() == "LISTENING") {
				for (auto key : currentlyPressed) {
					if (key.second && std::find(tempPressed.begin(), tempPressed.end(), key.first) == tempPressed.end()) {
						tempPressed.push_back(key.first);
					}
				}
			}
			else if (action.second.as<std::string>() == "FOUND") {
				if (tempPressed.size() > 0) {
					std::string tempStr = "";
					std::sort(tempPressed.begin(), tempPressed.end());
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
						pressed = false;
						break;
					}
				}
				if (pressed && wasPressedJoy[action.first] == false) {
					if (cnt > keyCount) { functionsToRun.clear(); }
					functionsToRun.emplace(action.first);
				}
			}
			wasPressedJoy[action.first] = pressed;
		}
		for (auto fnc : functionsToRun) {
			if (!iniConfig["OPTIONS"]["REQUIRE CLUTCH"].as<bool>() || wasPressedKb["CLUTCH"] || wasPressedJoy["CLUTCH"] || fnc == "SHOW MENU") {
				bindFunctions[fnc]();
			}
		}
		if (GetAsyncKeyState(VK_END) & 0x8000 && GetAsyncKeyState(VK_LCONTROL) & 0x8000 && GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
			DetachDLL();
		}

		nextFrameTime += std::chrono::milliseconds(16);
		std::this_thread::sleep_until(nextFrameTime);
	} while (keepAlive);
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
	keepAlive = false;
	Sleep(100);
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