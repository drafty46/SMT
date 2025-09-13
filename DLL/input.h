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
	class MouseListener : public OIS::MouseListener {
	public:
		bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID button) override;
		bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID button) override;
		bool mouseMoved(const OIS::MouseEvent& e) override;
	};
}

extern void InitInput();
extern void ShutdownInput();
extern OIS::InputManager* inputManager;
extern OIS::Keyboard* keyboard;
extern OIS::Mouse* mouse;
extern std::vector<OIS::JoyStick*> joystickList;
extern std::set<std::string> tempPressed;
extern std::atomic<bool> keepAliveInput;
extern std::atomic<int32_t> range;