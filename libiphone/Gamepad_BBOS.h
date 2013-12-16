#pragma once

#include <bps/event.h>
#include <screen/screen.h>
#include "Gamepad.h"

class Gamepad_BBOS
{
public:
	Gamepad_BBOS(screen_context_t screenCtx);
	virtual ~Gamepad_BBOS();

	virtual bool Initialize();
	virtual bool Shutdown();
	virtual bool IsConnected();
	virtual GamepadState GetState();

	void HandleEvent(bps_event_t * event);

private:
	bool Discover();
	bool ConnectTo(screen_device_t device);
	void Disconnect();
	void HandleEvent_Discover(screen_event_t event);
	void HandleEvent_Update(screen_event_t event);
	bool Poll();

	screen_context_t m_screenCtx;
	screen_device_t m_device;
	int m_type;
	int m_numButtons;
	int m_numAnalogs;
	GamepadState m_state;
};
