#include <SDL/SDL.h>
#include "Client.h"
#include "Controller.h"
#include "Debug.h"
#include "MyProtocols.h"
#include "Types.h"

Controller::Controller(int id, Client* client) : InputHandler(INPUT_PRIO_CONTROLLER)
{
	Assert(client);

	m_id = id;
	m_client = client;
}

void Controller::BindKey(int key, int actionID)
{
	Bind bind;

	bind.m_actionID = actionID;

	m_keyBinds[key] = bind;
}

void Controller::BindMouseAxis(INPUT_AXIS axis, int actionID)
{
	Bind bind;

	bind.m_actionID = actionID;

	m_mouseAxisBinds[axis] = bind;
}

void Controller::BindMouseButton(INPUT_BUTTON button, int actionID)
{
	Bind bind;

	bind.m_actionID = actionID;

	m_mouseButtonBinds[button] = bind;
}

void Controller::Update(Channel* channel)
{
}

bool Controller::OnEvent(Event& event)
{
	bool sent = false;

	if (event.type == EVT_KEY)
	{
		int key = event.key.key;

		if (m_keyBinds.count(key) > 0)
		{
			Bind& bind = m_keyBinds[key];
			float value = (float)event.key.state;

			SendAction(m_client->m_channel, bind.m_actionID, value);

			sent = true;
		}
	}

	if (event.type == EVT_MOUSEMOVE_ABS)
	{
		int axis = event.mouse_move.axis;

		if (m_mouseAxisBinds.count(axis) > 0)
		{
			Bind& bind = m_mouseAxisBinds[axis];
			float value = (float)event.mouse_move.position;

			SendAction(m_client->m_channel, bind.m_actionID, value);

			sent = true;
		}
	}

	if (event.type == EVT_MOUSEBUTTON)
	{
		int button = event.mouse_button.button;
		
		if (m_mouseButtonBinds.count(button) > 0)
		{
			Bind& bind = m_mouseButtonBinds[button];
			float value = (float)event.mouse_button.state;

			SendAction(m_client->m_channel, bind.m_actionID, value);

			sent = true;
		}
	}

	return sent;
}

void Controller::SendAction(Channel* channel, int in_actionID, float in_value) const
{
	Assert(channel);

	// Send change to self (client).
	if (0)
	{
		// FIXME: Packets get tangled up~

		PacketBuilder<8> packetBuilder;

		uint8_t protocolID = PROTOCOL_INPUT;
		uint8_t messageID = INPUTMSG_ACTION_LOCAL;
		uint16_t actionID = in_actionID;
		float value = in_value;

		packetBuilder.Write8(&protocolID);
		packetBuilder.Write8(&messageID);
		packetBuilder.Write16(&actionID);
		packetBuilder.Write32(&value);

		Packet packet = packetBuilder.ToPacket();
		channel->SendSelf(packet, channel->m_rtt / 2000);
	}

	// Send change to server.
	{
		PacketBuilder<8> packetBuilder;

		uint8_t protocolID = PROTOCOL_INPUT;
		uint8_t messageID = INPUTMSG_ACTION;
		uint16_t actionID = in_actionID;
		float value = in_value;

		packetBuilder.Write8(&protocolID);
		packetBuilder.Write8(&messageID);
		packetBuilder.Write16(&actionID);
		packetBuilder.Write32(&value);

		Packet packet = packetBuilder.ToPacket();
		channel->Send(packet);
	}
}
