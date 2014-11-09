#ifndef CLIENT_H
#define CLIENT_H
#pragma once

#include <vector>
#include "Channel.h"
#include "Controller.h"
#include "EntityPtr.h"
#include "ActionHandler.h"

class Engine;
//class Entity;
class Scene;

class Client
{
public:
	Client(Engine* engine);
	~Client();

	void Initialize(Channel* channel, bool clientSide);
	void SetActionHandler(ActionHandler* handler);

	bool m_clientSide;
	Channel* m_channel;
	Scene* m_clientScene;
	std::vector<RefEntity> m_entities;
	int m_repID;
	ActionHandler* m_actionHandler;

	// fixme/todo: make active controller property of client?
};

#endif
