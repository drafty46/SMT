#pragma once
#include "shared.h"

namespace SMT {
	class KeyListener : public OIS::KeyListener {
	public:
		bool keyPressed(const OIS::KeyEvent& e) override;
		bool keyReleased(const OIS::KeyEvent& e) override;
	};
	class JoyStickListener : public OIS::JoyStickListener {
	public:
		bool buttonPressed(const OIS::JoyStickEvent& e, int button) override;

		bool buttonReleased(const OIS::JoyStickEvent& e, int button) override;

		bool axisMoved(const OIS::JoyStickEvent& e, int axis) override;

		bool sliderMoved(const OIS::JoyStickEvent& e, int sliderID) override;

		bool povMoved(const OIS::JoyStickEvent& e, int pov) override;
	};
}

extern void InitInput();
extern void ShutdownInput();
extern OIS::InputManager* inputManager;
extern OIS::Keyboard* keyboard;
extern std::vector<OIS::JoyStick*> joystickList;