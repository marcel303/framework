#include "syscode_xinput.h"

#if BUILD_WITH_XINPUT

#include <Windows.h>
#include <Xinput.h>

static float APPLY_DEADZONE(SHORT v, SHORT t)
{
	SHORT va = v < 0 ? -v : +v;
	if (va <= t)
		return 0.f;
	else
	{
		float f;
		v = (va - t) * (v < 0 ? -1 : +1);
		f = v / (float)(32767 - t);
		f = f < -1.f ? -1.f : f > +1.f ? +1.f : f;
		return f;
	}
}

int SYS_PollXInput(unsigned char index, float * out_x, float * out_y, unsigned short * out_buttons)
{
	XINPUT_STATE state;
	DWORD result;

	ZeroMemory(&state, sizeof(XINPUT_STATE));
	result = XInputGetState(index, &state);

	if (result == ERROR_SUCCESS)
	{
		float x, y;
		WORD buttons = state.Gamepad.wButtons;

		x = 0.f;
		y = 0.f;

		x += APPLY_DEADZONE(+state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
		y += APPLY_DEADZONE(-state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
		x += APPLY_DEADZONE(+state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
		y += APPLY_DEADZONE(-state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
		
		if (0 != (buttons & XINPUT_GAMEPAD_DPAD_LEFT)) x -= 1;
		if (0 != (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)) x += 1;
		if (0 != (buttons & XINPUT_GAMEPAD_DPAD_UP)) y -= 1;
		if (0 != (buttons & XINPUT_GAMEPAD_DPAD_DOWN)) y += 1;

		x = x < -1.f ? -1.f : x > +1.f ? +1.f : x;
		y = y < -1.f ? -1.f : y > +1.f ? +1.f : y;
		
		*out_x = x;
		*out_y = y;

		*out_buttons = 0;

		if (0 != (buttons & XINPUT_GAMEPAD_A)) *out_buttons |= 1 << 0;
		if (0 != (buttons & XINPUT_GAMEPAD_B)) *out_buttons |= 1 << 1;

		return 1;
	}
	else
	{
		return 0;
	}
}

#endif
