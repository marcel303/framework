#ifndef CONTROLLER_H
#define CONTROLLER_H
#pragma once

#include <map>
#include "Channel.h"
#include "InputHandler.h"

class Client;

class Bind
{
public:
	inline Bind()
	{
		m_actionID = 0;
		m_value = 0;
	}

	int m_actionID;
	int m_value;
};

class Controller : public InputHandler
{
public:
	Controller(int id, Client* client);
	virtual ~Controller();

	void BindKey(int key, int actionID);
	void BindMouseAxis(INPUT_AXIS axis, int actionID);
	void BindMouseButton(INPUT_BUTTON button, int actionID);

	virtual bool OnEvent(Event& event);

	int GetID() const { return m_id; }

	void SetControllerIndex(int controllerIndex);
	int GetControllerIndex() const;

private:
	int m_id;
	int m_controllerIndex;

	void SendAction(Channel* channel, int in_actionID, float in_value) const;

	std::map<int, Bind> m_keyBinds;
	std::map<int, Bind> m_mouseAxisBinds;
	std::map<int, Bind> m_mouseButtonBinds;
	Client* m_client;
};

#endif
