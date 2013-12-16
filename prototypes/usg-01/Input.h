#pragma once

#include <allegro.h>
#include <deque>

enum InputType
{
	InputType_Undefined,
	InputType_TouchDown,
	InputType_TouchUp,
	InputType_TouchMove
};

class InputEvent
{
public:
	InputEvent()
	{
		type = InputType_Undefined;
		x = 0;
		y = 0;
		dx = 0;
		dy = 0;
	}

	InputEvent TranslateXY(int x, int y)
	{
		InputEvent result = *this;

		result.x -= x;
		result.y -= y;

		return result;
	}

	InputType type;
	int x;
	int y;
	int dx;
	int dy;
};

class InputMgr
{
public:
	InputMgr()
	{
		m_MouseButtons = 0;
		m_MousePos[0] = mouse_x;
		m_MousePos[1] = mouse_y;
	}

	void Update()
	{
		int mouseButtons = mouse_b;

		int change = mouseButtons ^ m_MouseButtons;
		int changeDown = change & ~m_MouseButtons;
		int changeUp = change & m_MouseButtons;
		m_MouseButtons = mouseButtons;

		if (changeDown & 0x1)
		{
			InputEvent e;
			e.type = InputType_TouchDown;
			e.x = mouse_x;
			e.y = mouse_y;
			Emit(e);
		}

		if (changeUp & 0x1)
		{
			InputEvent e;
			e.type = InputType_TouchUp;
			e.x = mouse_x;
			e.y = mouse_y;
			Emit(e);
		}

		int mouseDelta[2];

		mouseDelta[0] = mouse_x - m_MousePos[0];
		mouseDelta[1] = mouse_y - m_MousePos[1];

		if (mouseDelta[0] || mouseDelta[1])
		{
			m_MousePos[0] += mouseDelta[0];
			m_MousePos[1] += mouseDelta[1];

			InputEvent e;
			e.type = InputType_TouchMove;
			e.x = m_MousePos[0];
			e.y = m_MousePos[1];
			e.dx = mouseDelta[0];
			e.dy = mouseDelta[1];
			Emit(e);
		}
	}

	void Emit(InputEvent& e)
	{
		m_EventQueue.push_back(e);
	}

	bool Read(InputEvent& o_Event)
	{
		if (m_EventQueue.size() == 0)
			return false;

		o_Event = m_EventQueue.front();

		m_EventQueue.pop_front();

		return true;
	}

	int m_MouseButtons;
	int m_MousePos[2];
	std::deque<InputEvent> m_EventQueue;
};
