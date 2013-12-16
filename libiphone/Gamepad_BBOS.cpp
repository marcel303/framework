#include <bps/screen.h>
#include <string.h>
#include "Debugging.h"
#include "Gamepad_BBOS.h"
#include "Log.h"

#include <math.h> // fixme, remove

Gamepad_BBOS::Gamepad_BBOS(screen_context_t screenCtx)
	: m_screenCtx(screenCtx)
	, m_device(NULL)
	, m_numButtons(0)
	, m_numAnalogs(0)
{
}

Gamepad_BBOS::~Gamepad_BBOS()
{
	Shutdown();
}

bool Gamepad_BBOS::Initialize()
{
	LOG_INF("initializing gamepad", 0);

	if (Discover())
	{
		// yay!
	}

	LOG_INF("initializing gamepad [done]", 0);

	return true;
}

bool Gamepad_BBOS::Shutdown()
{
	if (m_device != NULL)
	{
		m_device = NULL;
	}

	return true;
}

bool Gamepad_BBOS::IsConnected()
{
	return m_device != NULL;
}

GamepadState Gamepad_BBOS::GetState()
{
	Assert(IsConnected());

	if (IsConnected())
	{
		Poll();

		return m_state;
	}
	else
	{
		GamepadState state;
		memset(&state, 0, sizeof(state));
		return state;
	}
}

void Gamepad_BBOS::HandleEvent(bps_event_t * event)
{
	int domain = bps_event_get_domain(event);

	if (domain == screen_get_domain())
	{
		int type = 0;

		screen_event_t screen_event = screen_event_get_event(event);

		if (screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &type) == 0)
		{
			switch (type)
			{
			case SCREEN_EVENT_DEVICE:
				HandleEvent_Discover(screen_event);
				break;

			case SCREEN_EVENT_GAMEPAD:
			case SCREEN_EVENT_JOYSTICK:
				HandleEvent_Update(screen_event);
				break;
			}
		}
	}
}

void Gamepad_BBOS::HandleEvent_Discover(screen_event_t event)
{
	// get device handle

	screen_device_t device = NULL;

	if (screen_get_event_property_pv(event, SCREEN_PROPERTY_DEVICE, (void**)&device) != 0)
	{
		LOG_ERR("failed to get device handle", 0);
		return;
	}

	// attached ?

	int attached;

	if (screen_get_event_property_iv(event, SCREEN_PROPERTY_ATTACHED, &attached) != 0)
	{
		LOG_ERR("failed to get device attachment state", 0);
		return;
	}

	if (attached)
	{
		// attachment

		int type = 0;

		if (screen_get_device_property_iv(device, SCREEN_PROPERTY_TYPE, &type) != 0)
		{
			LOG_ERR("failed to get device type", 0);
			return;
		}

		if (type != SCREEN_EVENT_GAMEPAD && type != SCREEN_EVENT_JOYSTICK)
			return;

		if (device != m_device)
		{
			if (m_device != NULL)
			{
				Disconnect();
			}

			ConnectTo(device);
		}
	}
	else
	{
		// detachment

		if (device == m_device)
		{
			Disconnect();
		}
	}
}

void Gamepad_BBOS::HandleEvent_Update(screen_event_t event)
{
	// do nothing. we'll use polling for now. we run at 60Hz, so we probably won't miss button presses

	LOG_DBG("received gamepad event", 0);
}

bool Gamepad_BBOS::Discover()
{
	LOG_INF("discovering gamepad device(s)", 0);

	// query the number of connected devices

	int deviceCount = 0;

	if (screen_get_context_property_iv(m_screenCtx, SCREEN_PROPERTY_DEVICE_COUNT, &deviceCount) != 0)
	{
		LOG_ERR("failed to query the number of connected devices", 0);
		return false;
	}

	// get a handle to each device

	screen_device_t * devices = new screen_device_t[deviceCount];

	if (screen_get_context_property_pv(m_screenCtx, SCREEN_PROPERTY_DEVICES, (void**)devices) != 0)
	{
		LOG_ERR("failed to get handles to the connected devices", 0);
		return false;
	}

	// connect to the first gamepad or joystick we find

	for (int i = 0; i < deviceCount; i++)
	{
		int type;

		if (screen_get_device_property_iv(devices[i], SCREEN_PROPERTY_TYPE, &type) != 0)
		{
			LOG_ERR("failed to get the device type", 0);
			continue;
		}
		
		if (type != SCREEN_EVENT_GAMEPAD && type != SCREEN_EVENT_JOYSTICK)
			continue;

		if (ConnectTo(devices[i]))
		{
			break;
		}
    }

	delete [] devices;
	devices = 0;

	LOG_INF("discovering gamepad device(s) [done]", 0);

	return true;
}

bool Gamepad_BBOS::ConnectTo(screen_device_t device)
{
	LOG_INF("connecting to gamepad device", 0);

	// query device properties

	int type;
	int numAnalogs = 0;
	int numButtons = 0;

	if (screen_get_device_property_iv(device, SCREEN_PROPERTY_TYPE, &type) != 0)
	{
		LOG_ERR("failed to query device type", 0);
		return false;
	}

	if (screen_get_device_property_iv(device, SCREEN_PROPERTY_BUTTON_COUNT, &numButtons) != 0)
	{
		LOG_ERR("failed to query button count", 0);
		numButtons = 0;
	}

	// check for the existence of analog sticks

	int analog[3] = {};

	if (screen_get_device_property_iv(device, SCREEN_PROPERTY_ANALOG0, analog) == 0)
	{
		numAnalogs++;
	}

	if (screen_get_device_property_iv(device, SCREEN_PROPERTY_ANALOG1, analog) == 0)
	{
		numAnalogs++;
	}

#ifdef DEBUG
	char name[128];

	if (screen_get_device_property_cv(device, SCREEN_PROPERTY_ID_STRING, sizeof(name), name) == 0)
	{
		LOG_DBG("connected to %s", name);
	}
#endif

	LOG_INF("gamepad has %d analog inputs", numAnalogs);

	m_type = type;
	m_numButtons = numButtons;
	m_numAnalogs = numAnalogs;

	//

	m_device = device;

	// clear current state

	memset(&m_state, 0, sizeof(m_state));

	LOG_INF("connecting to gamepad device [done]", 0);

	return true;
}

void Gamepad_BBOS::Disconnect()
{
	Assert(m_device != NULL);

	if (m_device != NULL)
	{
		LOG_INF("disconnecting gamepad device", 0);

		m_device = NULL;

		LOG_INF("disconnecting gamepad device [done]", 0);
	}
}

static const int DEADZONE = 0;

static int ConvertAnalog(int value)
{
	if (value < -127)
		value = -127;
	if (value > 127)
		value = 127;
	if (value >= -DEADZONE && value <= +DEADZONE)
		value = 0;
	return value * 256 * 128 / 127;
}

static void PollAnalog(screen_device_t device, int index, GamepadState & state)
{
	Assert(device != NULL);
	Assert(index <= 1);

	const int prop[] =
	{
		SCREEN_PROPERTY_ANALOG0,
		SCREEN_PROPERTY_ANALOG1
	};

	int analog[3] = {};

	if (screen_get_device_property_iv(device, prop[index], analog) != 0)
	{
		LOG_ERR("failed to poll analog state", 0);
	}
	else
	{
		// update analog

		if (index == 0)
		{
			state.analogs[GamepadAnalog_LeftX] = ConvertAnalog(analog[0]);
			state.analogs[GamepadAnalog_LeftY] = ConvertAnalog(analog[1]);
		}
		
		if (index == 1)
		{
			state.analogs[GamepadAnalog_RightX] = ConvertAnalog(analog[0]);
			state.analogs[GamepadAnalog_RightY] = ConvertAnalog(analog[1]);
		}

		LOG_DBG("analog input %d: %d, %d", index, analog[0], analog[1]);
	}
}

bool Gamepad_BBOS::Poll()
{
	Assert(m_device != NULL);

	if (m_device != NULL)
	{
		memset(&m_state, 0, sizeof(m_state));

		int buttons = 0;

		if (screen_get_device_property_iv(m_device, SCREEN_PROPERTY_BUTTONS, &buttons) != 0)
		{
			LOG_ERR("failed to poll button state", 0);
		}
		else
		{
			// update buttons

			m_state.buttons[GamepadButton_Left_Left    ] = (buttons & SCREEN_DPAD_LEFT_GAME_BUTTON ) != 0;
			m_state.buttons[GamepadButton_Left_Right   ] = (buttons & SCREEN_DPAD_RIGHT_GAME_BUTTON) != 0;
			m_state.buttons[GamepadButton_Left_Up      ] = (buttons & SCREEN_DPAD_UP_GAME_BUTTON   ) != 0;
			m_state.buttons[GamepadButton_Left_Down    ] = (buttons & SCREEN_DPAD_DOWN_GAME_BUTTON ) != 0;
			m_state.buttons[GamepadButton_Left_Trigger ] = (buttons & SCREEN_L1_GAME_BUTTON) != 0;

			m_state.buttons[GamepadButton_Right_Left   ] = (buttons & SCREEN_X_GAME_BUTTON) != 0;
			m_state.buttons[GamepadButton_Right_Right  ] = (buttons & SCREEN_B_GAME_BUTTON) != 0;
			m_state.buttons[GamepadButton_Right_Up     ] = (buttons & SCREEN_Y_GAME_BUTTON) != 0;
			m_state.buttons[GamepadButton_Right_Down   ] = (buttons & SCREEN_A_GAME_BUTTON) != 0;
			m_state.buttons[GamepadButton_Right_Trigger] = (buttons & SCREEN_R1_GAME_BUTTON) != 0;

			m_state.buttons[GamepadButton_Select       ] = (buttons & SCREEN_MENU1_GAME_BUTTON) != 0;
			m_state.buttons[GamepadButton_Start        ] = (buttons & SCREEN_MENU2_GAME_BUTTON) != 0;
		}

		for (int i = 0; i < m_numAnalogs; ++i)
		{
			PollAnalog(m_device, i, m_state);
		}

	#ifdef DEBUG // fake input!
		static float t = 0.f;
		t += 1.0f / 60.0f;
		//m_state.analogs[GamepadAnalog_RightX] = ConvertAnalog(127.f * sinf(t * 2.f));
		//m_state.analogs[GamepadAnalog_RightY] = ConvertAnalog(127.f * cosf(t * 2.f));
		m_state.analogs[GamepadAnalog_LeftX] = ConvertAnalog(127.f * sinf(t * 1.12f));
		m_state.analogs[GamepadAnalog_LeftY] = ConvertAnalog(127.f * cosf(t * 1.23f));
	#endif

		return true;
	}
	else
	{
		return false;
	}
}
