#include <string>
#include "input.h"

Keyboard gKeyboard;
Mouse gMouse;

Keyboard::Keyboard()
{
	//memset(mKeyState, 0, sizeof(int) * KeyCode__Count);
	memset(mKeyState, 0, sizeof(mKeyState));
}

void Keyboard::Set(KeyCode code, int state)
{
	mKeyState[code] = state;
}

int Keyboard::Get(KeyCode code)
{
	return mKeyState[code];
}

void Keyboard::TiltSet(Vec2F tilt)
{
	mTilt = tilt;
}

Vec2F Keyboard::TiltGet()
{
	return mTilt;
}

//

Mouse::Mouse()
{
	X = 0;
	Y = 0;
}

void Mouse::Set(float x, float y)
{
	X = x;
	Y = y;
}
