#pragma once

#include "Types.h"

enum KeyCode
{
	KeyCode_Undefined,
	KeyCode_Left,
	KeyCode_Right,
	KeyCode_Up,
	KeyCode_Down,
	KeyCode_Escape,
	KeyCode__Count
};

class Keyboard
{
public:
	Keyboard();

	void Set(KeyCode code, int state);
	int Get(KeyCode code);
	void TiltSet(Vec2F tilt);
	Vec2F TiltGet();

private:
	int mKeyState[KeyCode__Count];
	Vec2F mTilt;
};

extern Keyboard gKeyboard;

class Mouse
{
public:
	Mouse();

	void Set(float x, float y);

	float X;
	float Y;
};

extern Mouse gMouse;
