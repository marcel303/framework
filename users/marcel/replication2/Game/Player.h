#ifndef PLAYER_H
#define PLAYER_H
#pragma once

#include "ControllerExample.h"
#include "EntityBrick.h"
#include "EntityPlayer.h"
#include "Mesh.h" //fixme
#include "PlayerControl.h"
#include "ResSnd.h"
#include "ResSndSrc.h"
#include "Weapon.h"

class Player : public EntityPlayer
{
	class Player_NS : public NetSerializable
	{
		Player * m_owner;

	public:
		Player_NS(Player * owner)
			: NetSerializable(owner)
			, m_owner(owner)
			, m_color(this)
		{
		}

		virtual void SerializeStruct()
		{
			if (IsInit())
			{
			}
		}

		NetValue<uint32_t> m_color;
	};

public:
	Player(Client* client, InputManager* inputMgr);
	virtual ~Player();
	virtual void PostCreate();

	virtual Mat4x4 GetTransform() const;

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void Render();

	void SetFort(ShEntity brick);  // fixme, remove forwarddecl

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);
	virtual void OnAction(int actionID, float value);
	virtual void OnMessage(int message, int value);

	virtual float GetFOV() const;
	virtual Mat4x4 GetCamera() const;

//fixme private:
	const static int MSG_PLAYSOUND = 0;
	const static int SND_JUMP = 0;
	const static int SND_HURT = 1;

	ControllerExample m_controllerExample;

	// State.
	Player_NS * m_player_NS;

	ShWeapon m_weapon;
	EntityLink<Entity> m_weaponLink;
	EntityLink<EntityBrick> m_fort;
	
	// Controls.
	PlayerControl * m_control;

	// Visuals.
	Mesh m_mesh;

	// Sounds.
	ShSndSrc m_sndSrc;

	ShSnd m_sndJump;
	ShSnd m_sndHurt;
};

#endif
