#ifndef ENTITYPLAYER_H
#define ENTITYPLAYER_H
#pragma once

#include "Client.h"
#include "Entity.h"
#include "ActionHandler.h"
#include "InputManager.h"
#include "Vec3.h"

class EntityPlayer : public Entity, public ActionHandler
{
public:
	EntityPlayer();
	virtual ~EntityPlayer();

	void Initialize(Client* client, InputManager* inputMgr);

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);

	virtual void Render();

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

	virtual void OnActivate();
	virtual void OnDeActivate();

	virtual void OnAction(int actionID, float value);
	virtual void OnMessage(int message, int value);

	virtual float GetFOV() const;
	virtual Mat4x4 GetCamera() const;
	virtual Mat4x4 GetPerspective() const;
	Vec3 GetCameraPosition() const;
	Vec3 GetCameraOrientation() const;

	virtual void SetController(int id);
	Controller * GetController() { return m_controller; }

protected:
	bool m_activated;
	InputManager* m_inputMgr;
	Controller* m_controller;
	std::vector<Controller*> m_controllers;
};

#endif
