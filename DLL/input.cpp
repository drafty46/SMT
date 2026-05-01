#include "shared.h"
#include "input.h"
#include "config.h"
#include "gui.h"
#include "game_data.h"
#include "memory.h"
#include <mutex>
#include <Xinput.h>

extern std::unordered_map <Vehicle*, std::atomic<bool>> IsInAuto;

OIS::InputManager* inputManager;
std::atomic<bool> keepAliveInput = true;
OIS::Keyboard* keyboard = nullptr;
std::vector<OIS::JoyStick*> joystickList;
OIS::Mouse* mouse = nullptr;
std::unordered_map<std::string, bool> currentlyPressed;
std::set<std::string> tempPressed;
std::unordered_map<std::string, bool> wasPressedKb;
std::unordered_map<std::string, bool> wasPressedJoy;
std::atomic<int32_t> range = 0;

std::string abbreviate(const std::string& input);

namespace {
	constexpr int32_t CONTROLLER_AXIS_THRESHOLD = 20000;
	std::mutex tempPressedMutex;

	enum class InputGroup {
		Keyboard,
		Controller
	};

	std::set<std::string> tempPressedKeyboard;
	std::set<std::string> tempPressedController;

	std::set<std::string>& TempPressedFor(InputGroup group) {
		return group == InputGroup::Keyboard ? tempPressedKeyboard : tempPressedController;
	}

	void AddTempPressed(InputGroup group, const std::string& entry) {
		std::lock_guard<std::mutex> lock(tempPressedMutex);
		TempPressedFor(group).emplace(entry);
	}

	void SetPressed(InputGroup group, const std::string& entry, bool pressed) {
		if (pressed && !currentlyPressed[entry]) {
			AddTempPressed(group, entry);
		}
		currentlyPressed[entry] = pressed;
	}

	std::string ControllerPrefix(const OIS::Object* device) {
		return abbreviate(device->vendor());
	}

	std::string ConsumeTempPressed(InputGroup group, bool noneWhenEmpty = true) {
		std::lock_guard<std::mutex> lock(tempPressedMutex);
		std::set<std::string>& tempPressedSet = TempPressedFor(group);
		if (tempPressedSet.empty()) {
			return noneWhenEmpty ? "NONE" : "";
		}

		std::string tempStr;
		for (const auto& key : tempPressedSet) {
			tempStr += key;
			tempStr += "+";
		}
		tempStr.pop_back();
		tempPressedSet.clear();
		return tempStr;
	}

	void PollJoyStickState(const OIS::JoyStick* joystick) {
		const std::string controller = ControllerPrefix(joystick);
		const OIS::JoyStickState& state = joystick->getJoyStickState();

		for (size_t button = 0; button < state.mButtons.size(); ++button) {
			SetPressed(InputGroup::Controller, controller + ".b." + std::to_string(button), state.mButtons[button]);
		}

		for (size_t axis = 0; axis < state.mAxes.size(); ++axis) {
			SetPressed(InputGroup::Controller, controller + ".a." + std::to_string(axis) + ".p", state.mAxes[axis].abs > CONTROLLER_AXIS_THRESHOLD);
			SetPressed(InputGroup::Controller, controller + ".a." + std::to_string(axis) + ".n", state.mAxes[axis].abs < -CONTROLLER_AXIS_THRESHOLD);
		}

		for (int32_t slider = 0; slider < 4; ++slider) {
			SetPressed(InputGroup::Controller, controller + ".s.x." + std::to_string(slider) + ".p", state.mSliders[slider].abX > CONTROLLER_AXIS_THRESHOLD);
			SetPressed(InputGroup::Controller, controller + ".s.x." + std::to_string(slider) + ".n", state.mSliders[slider].abX < -CONTROLLER_AXIS_THRESHOLD);
			SetPressed(InputGroup::Controller, controller + ".s.y." + std::to_string(slider) + ".p", state.mSliders[slider].abY > CONTROLLER_AXIS_THRESHOLD);
			SetPressed(InputGroup::Controller, controller + ".s.y." + std::to_string(slider) + ".n", state.mSliders[slider].abY < -CONTROLLER_AXIS_THRESHOLD);
		}

		for (int32_t pov = 0; pov < 4; ++pov) {
			SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".up", (state.mPOV[pov].direction & OIS::Pov::North) != 0);
			SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".down", (state.mPOV[pov].direction & OIS::Pov::South) != 0);
			SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".right", (state.mPOV[pov].direction & OIS::Pov::East) != 0);
			SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".left", (state.mPOV[pov].direction & OIS::Pov::West) != 0);
		}
	}

	void SetXInputButton(DWORD userIndex, const std::string& name, WORD buttons, WORD mask) {
		SetPressed(InputGroup::Controller, "Xi." + std::to_string(userIndex) + ".b." + name, (buttons & mask) != 0);
	}

	void SetXInputAxis(DWORD userIndex, const std::string& name, int32_t value) {
		SetPressed(InputGroup::Controller, "Xi." + std::to_string(userIndex) + ".a." + name + ".p", value > CONTROLLER_AXIS_THRESHOLD);
		SetPressed(InputGroup::Controller, "Xi." + std::to_string(userIndex) + ".a." + name + ".n", value < -CONTROLLER_AXIS_THRESHOLD);
	}

	void PollXInputController(DWORD userIndex) {
		XINPUT_STATE state = {};
		const bool connected = XInputGetState(userIndex, &state) == ERROR_SUCCESS;
		const WORD buttons = connected ? state.Gamepad.wButtons : 0;
		const BYTE leftTrigger = connected ? state.Gamepad.bLeftTrigger : 0;
		const BYTE rightTrigger = connected ? state.Gamepad.bRightTrigger : 0;

		SetXInputButton(userIndex, "DU", buttons, XINPUT_GAMEPAD_DPAD_UP);
		SetXInputButton(userIndex, "DD", buttons, XINPUT_GAMEPAD_DPAD_DOWN);
		SetXInputButton(userIndex, "DL", buttons, XINPUT_GAMEPAD_DPAD_LEFT);
		SetXInputButton(userIndex, "DR", buttons, XINPUT_GAMEPAD_DPAD_RIGHT);
		SetXInputButton(userIndex, "START", buttons, XINPUT_GAMEPAD_START);
		SetXInputButton(userIndex, "BACK", buttons, XINPUT_GAMEPAD_BACK);
		SetXInputButton(userIndex, "LS", buttons, XINPUT_GAMEPAD_LEFT_THUMB);
		SetXInputButton(userIndex, "RS", buttons, XINPUT_GAMEPAD_RIGHT_THUMB);
		SetXInputButton(userIndex, "LB", buttons, XINPUT_GAMEPAD_LEFT_SHOULDER);
		SetXInputButton(userIndex, "RB", buttons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
		SetXInputButton(userIndex, "A", buttons, XINPUT_GAMEPAD_A);
		SetXInputButton(userIndex, "B", buttons, XINPUT_GAMEPAD_B);
		SetXInputButton(userIndex, "X", buttons, XINPUT_GAMEPAD_X);
		SetXInputButton(userIndex, "Y", buttons, XINPUT_GAMEPAD_Y);

		SetPressed(InputGroup::Controller, "Xi." + std::to_string(userIndex) + ".t.L", leftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		SetPressed(InputGroup::Controller, "Xi." + std::to_string(userIndex) + ".t.R", rightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

		SetXInputAxis(userIndex, "LX", connected ? state.Gamepad.sThumbLX : 0);
		SetXInputAxis(userIndex, "LY", connected ? state.Gamepad.sThumbLY : 0);
		SetXInputAxis(userIndex, "RX", connected ? state.Gamepad.sThumbRX : 0);
		SetXInputAxis(userIndex, "RY", connected ? state.Gamepad.sThumbRY : 0);
	}

	void PollXInputControllers() {
		for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
			PollXInputController(userIndex);
		}
	}
}

void ClearTempPressed() {
	std::lock_guard<std::mutex> lock(tempPressedMutex);
	tempPressedKeyboard.clear();
	tempPressedController.clear();
}

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
		SetPressed(InputGroup::Keyboard, entry, true);
		return true;
	}

	bool KeyListener::keyReleased(const OIS::KeyEvent& e) {
		SetPressed(InputGroup::Keyboard, "Kb." + std::to_string(e.key), false);
		return true;
	}

	bool JoyStickListener::buttonPressed(const OIS::JoyStickEvent& e, int button) {
		std::string entry = ControllerPrefix(e.device) + ".b." + std::to_string(button);
		SetPressed(InputGroup::Controller, entry, true);
		return true;
	}

	bool JoyStickListener::buttonReleased(const OIS::JoyStickEvent& e, int button) {
		SetPressed(InputGroup::Controller, ControllerPrefix(e.device) + ".b." + std::to_string(button), false);
		return true;
	}

	bool JoyStickListener::axisMoved(const OIS::JoyStickEvent& e, int axis) {
		std::string controller = ControllerPrefix(e.device);
		SetPressed(InputGroup::Controller, controller + ".a." + std::to_string(axis) + ".p", e.state.mAxes[axis].abs > CONTROLLER_AXIS_THRESHOLD);
		SetPressed(InputGroup::Controller, controller + ".a." + std::to_string(axis) + ".n", e.state.mAxes[axis].abs < -CONTROLLER_AXIS_THRESHOLD);
		return true;
	}

	bool JoyStickListener::sliderMoved(const OIS::JoyStickEvent& e, int sliderID) {
		std::string controller = ControllerPrefix(e.device);
		SetPressed(InputGroup::Controller, controller + ".s.x." + std::to_string(sliderID) + ".p", e.state.mSliders[sliderID].abX > CONTROLLER_AXIS_THRESHOLD);
		SetPressed(InputGroup::Controller, controller + ".s.x." + std::to_string(sliderID) + ".n", e.state.mSliders[sliderID].abX < -CONTROLLER_AXIS_THRESHOLD);
		SetPressed(InputGroup::Controller, controller + ".s.y." + std::to_string(sliderID) + ".p", e.state.mSliders[sliderID].abY > CONTROLLER_AXIS_THRESHOLD);
		SetPressed(InputGroup::Controller, controller + ".s.y." + std::to_string(sliderID) + ".n", e.state.mSliders[sliderID].abY < -CONTROLLER_AXIS_THRESHOLD);
		return true;
	}

	bool JoyStickListener::povMoved(const OIS::JoyStickEvent& e, int pov) {
		std::string controller = ControllerPrefix(e.device);
		SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".up", (e.state.mPOV[pov].direction & OIS::Pov::North) != 0);
		SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".down", (e.state.mPOV[pov].direction & OIS::Pov::South) != 0);
		SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".right", (e.state.mPOV[pov].direction & OIS::Pov::East) != 0);
		SetPressed(InputGroup::Controller, controller + ".p." + std::to_string(pov) + ".left", (e.state.mPOV[pov].direction & OIS::Pov::West) != 0);
		return true;
	}

	bool MouseListener::mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID button) {
		std::string entry = "Ms." + std::to_string(button);
		if ((int)button > 1) {
			SetPressed(InputGroup::Keyboard, entry, true);
		}
		return true;
	}

	bool MouseListener::mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID button) {
		if ((int)button > 1) {
			SetPressed(InputGroup::Keyboard, "Ms." + std::to_string(button), false);
		}
		return true;
	}

	bool MouseListener::mouseMoved(const OIS::MouseEvent& e) {
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
					PollJoyStickState(js);
				}
			}
			PollXInputControllers();
			mouse->capture();
			bool goToNeutral = iniConfig["OPTIONS"]["REQUIRE GEAR HELD"].as<bool>();
			for (auto action : iniConfig["KEYBOARD"]) {
				bool pressed = true;
				std::string binding = action.second.as<std::string>();
				if (binding == "FOUND") {
					iniConfig["KEYBOARD"][action.first] = ConsumeTempPressed(InputGroup::Keyboard);
				}
				else if (binding == "LISTENING") {
					std::string captured = ConsumeTempPressed(InputGroup::Keyboard, false);
					if (!captured.empty()) {
						iniConfig["KEYBOARD"][action.first] = captured;
					}
					pressed = false;
				}
				else {
					int32_t cnt = 0;
					for (auto part : binding | std::views::split('+')) {
						cnt++;
						if (!currentlyPressed[std::string(part.begin(), part.end())]) {
							std::string partStr(part.begin(), part.end());
							if (action.first.starts_with("GEAR") && binding != "NONE") {
								if ((partStr == iniConfig["KEYBOARD"]["RANGE HIGH"].as<std::string>() && range == 1) ||
									(partStr == iniConfig["KEYBOARD"]["RANGE LOW"].as<std::string>() && range == -1)) {
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
				std::string binding = action.second.as<std::string>();
				if (binding == "FOUND") {
					iniConfig["CONTROLLER"][action.first] = ConsumeTempPressed(InputGroup::Controller);
				}
				else if (binding == "LISTENING") {
					std::string captured = ConsumeTempPressed(InputGroup::Controller, false);
					if (!captured.empty()) {
						iniConfig["CONTROLLER"][action.first] = captured;
					}
					pressed = false;
				}
				else {
					int32_t cnt = 0;
					for (auto part : binding | std::views::split('+')) {
						cnt++;
						if (!currentlyPressed[std::string(part.begin(), part.end())]) {
							std::string partStr(part.begin(), part.end());
							if (action.first.starts_with("GEAR") && binding != "NONE") {
								if ((partStr == iniConfig["CONTROLLER"]["RANGE HIGH"].as<std::string>() && range == 1) ||
									(partStr == iniConfig["CONTROLLER"]["RANGE LOW"].as<std::string>() && range == -1)) {
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
	mouse = static_cast<OIS::Mouse*>(inputManager->createInputObject(OIS::OISMouse, true));

	SMT::KeyListener* myKeyListener = new SMT::KeyListener();
	keyboard->setEventCallback(myKeyListener);
	SMT::JoyStickListener* myJoyStickListener = new SMT::JoyStickListener();
	for (auto& js : joystickList) {
		js->setEventCallback(myJoyStickListener);
	}
	SMT::MouseListener* myMouseListener = new SMT::MouseListener();
	mouse->setEventCallback(myMouseListener);

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
		if (mouse) {
			inputManager->destroyInputObject(mouse);
		}
		OIS::InputManager::destroyInputSystem(inputManager);
	}
}
