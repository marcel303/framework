#include "Debug.h"
#include "EntityPlayer.h" // fixme, revise
#include "InputManager.h"
#include "MyProtocols.h"

InputManager::InputManager()
{
}

InputManager::~InputManager()
{
}

void InputManager::SV_Update()
{
	// TODO: Poll devices.
}

void InputManager::SV_AddClient(Client* client)
{
	Assert(client);

	m_serverClients[client->m_channel] = client;
}

void InputManager::CL_AddClient(Client* client)
{
	Assert(client);

	m_clientClients[client->m_channel] = client;
}

void InputManager::SV_RemoveClient(Client* client)
{
	Assert(client);

	m_serverClients.erase(client->m_channel);
}

void InputManager::CL_RemoveClient(Client* client)
{
	Assert(client);

	m_clientClients.erase(client->m_channel);
}

void InputManager::OnReceive(Packet& packet, Channel* channel)
{
	uint8_t messageID;

	if (!packet.Read8(&messageID))
		return;

	switch (messageID)
	{
	case INPUTMSG_ACTION:
		HandleAction(packet, channel);
		break;
	case INPUTMSG_ACTION_LOCAL:
		HandleActionLocal(packet, channel);
		break;
		/*
	case INPUTMSG_SETCONTROLLER:
		HandleSetController(packet, channel);
		break;*/
	}
}

void InputManager::CL_AddInputHandler(InputHandler* handler)
{
	Assert(handler);

	m_clientInputHandlers.push_back(handler);
}

void InputManager::OnEvent(Event& event)
{
	bool captured = false;

	// TODO: Process GUI controller.
	for (InputHandlerCollectionItr i = m_clientInputHandlers.begin(); i != m_clientInputHandlers.end(); ++i)
	{
		if (captured == false)
		{
			if ((*i)->OnEvent(event))
				captured = true;
		}
	}

	for (ClientCollectionItr i = m_clientClients.begin(); i != m_clientClients.end(); ++i)
	{
		if (captured == false)
		{
			if (!i->second->m_clientScene->m_activeEntity.expired())
			{
				EntityPlayer* player = static_cast<EntityPlayer*>(i->second->m_clientScene->m_activeEntity.lock().get());

				if (player->GetController())
				{
					if (player->GetController()->OnEvent(event))
						captured = true;
				}
			}
		}
	}
}

void InputManager::HandleAction(Packet& packet, Channel* channel)
{
	Client* client = SV_FindClient(channel);

	if (client)
	{
		uint16_t actionID;
		float value;

		if (!packet.Read16(&actionID))
			return;
		if (!packet.Read32(&value))
			return;

		if (client->m_actionHandler)
			client->m_actionHandler->OnAction(actionID, value);
	}
	else
	{
		DB_ERR("received input message for non-existing client");
	}
}

void InputManager::HandleActionLocal(Packet& packet, Channel* channel)
{
	Client* client = CL_FindClient(channel);

	if (client)
	{
		uint16_t actionID;
		float value;

		if (!packet.Read16(&actionID))
			return;
		if (!packet.Read32(&value))
			return;

		if (client->m_actionHandler)
			client->m_actionHandler->OnAction(actionID, value);
	}
	else
	{
		DB_ERR("received input message for non-existing client");
	}
}

/*
void InputManager::HandleSetController(Packet& packet, Channel* channel)
{
	Client* client = CL_FindClient(channel);

	if (client)
	{
		uint32_t controllerID;

		if (!packet.Read32(&controllerID))
			return;

		Controller* controller = 0;

		for (int i = 0; i < client->m_controllers.size(); ++i)
			if (client->m_controllers[i]->m_id == controllerID)
				controller = client->m_controllers[i];

		client->m_controller = controller;
	}
	else
	{
		DB_ERR("received input message for non-existing client");
	}
}
*/

Client* InputManager::SV_FindClient(Channel* channel)
{
	Assert(channel);

	ClientCollectionItr i;

	i = m_serverClients.find(channel);

	if (i == m_serverClients.end())
		return 0;
	else
		return i->second;
}

Client* InputManager::CL_FindClient(Channel* channel)
{
	Assert(channel);

	ClientCollectionItr i;

	i = m_clientClients.find(channel);

	if (i == m_clientClients.end())
		return 0;
	else
		return i->second;
}
