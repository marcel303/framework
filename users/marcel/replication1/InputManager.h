#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H
#pragma once

#include <map>
#include "Channel.h"
#include "Client.h"
#include "EventHandler.h"
#include "PacketListener.h"

class InputManager : public PacketListener, public EventHandler
{
public:
	InputManager();
	~InputManager();

	void SV_Update();

	void SV_AddClient(Client* client);
	void CL_AddClient(Client* client);
	void SV_RemoveClient(Client* client);
	void CL_RemoveClient(Client* client);

	// NOTE: Only for adding GUI controllers.. nothing else.
	// TODO: Need a list of controllers (per player) and select it based on ID.
	// Add list of controllers to InputManager, clear it on player remove.
	void CL_AddInputHandler(InputHandler* handler);

	virtual void OnReceive(Packet& packet, Channel* channel);
	virtual void OnEvent(Event& event);

private:
	void HandleAction(Packet& packet, Channel* channel);
	void HandleActionLocal(Packet& packet, Channel* channel);
	void HandleSetController(Packet& packet, Channel* channel);

	Client* SV_FindClient(Channel* channel);
	Client* CL_FindClient(Channel* channel);

	typedef std::map<Channel*, Client*> ClientCollection;
	typedef ClientCollection::iterator ClientCollectionItr;

	ClientCollection m_serverClients;
	ClientCollection m_clientClients;

	typedef std::vector<InputHandler*> InputHandlerCollection;
	typedef InputHandlerCollection::iterator InputHandlerCollectionItr;

	InputHandlerCollection m_clientInputHandlers;
};

#endif
