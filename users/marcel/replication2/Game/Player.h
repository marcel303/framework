#ifndef PLAYER_H
#define PLAYER_H
#pragma once

#include "ControllerExample.h"
#include "EntityBrick.h"
#include "EntityPlayer.h"
#include "Mesh.h"
#include "PlayerControl.h"
#include "ResSnd.h"
#include "ResSndSrc.h"
#include "Weapon.h"

class Player : public EntityPlayer
{
	IMPLEMENT_ENTITY(Player);

	class Player_NS : public NetSerializable
	{
		uint16_t m_owningChannelID;
		uint32_t m_color;

	public:
		Player_NS(Player * owner, uint16_t owningChannelID)
			: NetSerializable(owner)
			, m_owningChannelID(owningChannelID)
			, m_color(0)
		{
		}

		virtual void SerializeStruct()
		{
			Serialize(m_owningChannelID);
			Serialize(m_color);
		}

		uint16_t GetOwningChannelID() const
		{
			return m_owningChannelID;
		}

		uint32_t GetColor() const
		{
			return m_color;
		}

		void SetColor(uint32_t color)
		{
			m_color = color;
		}
	};

public:
	Player(uint16_t owningChannelID = 0);
	virtual ~Player();

	virtual Mat4x4 GetTransform() const;

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void Render();

	void SetFort(ShEntity brick);

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

	ControllerExample* m_controllerExample;

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
