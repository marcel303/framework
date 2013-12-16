#pragma once

enum GamepadAnalog
{
	GamepadAnalog_LeftX,
	GamepadAnalog_LeftY,
	GamepadAnalog_RightX,
	GamepadAnalog_RightY
};

enum GamepadButton
{
	GamepadButton_Left_Left, // pos/neg
	GamepadButton_Left_Right,
	GamepadButton_Left_Up,
	GamepadButton_Left_Down,
	GamepadButton_Left_Trigger,
	GamepadButton_Right_Left, // pos/neg
	GamepadButton_Right_Right,
	GamepadButton_Right_Up,
	GamepadButton_Right_Down,
	GamepadButton_Right_Trigger,
	GamepadButton_Select,
	GamepadButton_Start,
	GamepadButtonCount
};

class GamepadState
{
public:
	static const int kNumAnalogs = 4;
	static const int kNumButtons = GamepadButtonCount;

	int analogs[kNumAnalogs]; // -32768..+32767
	int buttons[kNumButtons];
};

class Gamepad
{
public:
	virtual ~Gamepad();

	virtual bool Initialize() = 0;
	virtual bool Shutdown() = 0;
	virtual bool IsConnected() = 0;
	virtual GamepadState GetState() = 0;
};
