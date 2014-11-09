#ifndef ENTITYBRICK_H
#define ENTITYBRICK_H
#pragma once

#include "Calc.h"
#include "CDCube.h"
#include "Entity.h"
#include "Mesh.h"
#include "ResPS.h"
#include "ResVS.h"
#include "Vec3.h"

class EntityBrick : public Entity
{
	class Brick_NS : public NetSerializable
	{
		int8_t m_fort;
		int8_t m_hp;
		float m_life;

	public:
		Brick_NS(EntityBrick * owner, int8_t fort, int8_t hp, float life)
			: NetSerializable(owner)
			, m_fort(fort)
			, m_hp(hp)
			, m_life(life)
		{
		}

		virtual void SerializeStruct()
		{
			if (IsInit())
			{
				EntityBrick * brick = static_cast<EntityBrick*>(GetOwner());

				Serialize(brick->m_size[0]);
				Serialize(brick->m_size[1]);
				Serialize(brick->m_size[2]);
				Serialize(brick->m_indestructible);
			}

			Serialize(m_fort);
			Serialize(m_hp);
			Serialize(m_life);
		}

		void ChangeHP(int amount)
		{
			const int newHP = Calc::Clamp(m_hp - amount, 0, MAX_HP);
			if (newHP != m_hp)
			{
				m_hp = newHP;
				SetDirty();
			}
		}

		int GetHP() const
		{
			return m_hp;
		}

		void ChangeFort(int change)
		{
			m_fort += change;
			SetDirty();
		}

		int GetFort() const
		{
			return m_fort;
		}

		void ChangeLife(float amount)
		{
			const float newLife = Calc::Clamp(m_life + amount, 0.f, 1.f);

			if (newLife != m_life)
			{
				m_life = newLife;
				SetDirty();
			}
		}

		float GetLife() const
		{
			return m_life;
		}
	};

public:
	EntityBrick();
	virtual ~EntityBrick();

	void Initialize(Vec3 position, Vec3 size, bool indestructible);

	virtual Mat4x4 GetTransform() const;

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();
	virtual void Render();

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

	virtual void ScriptDamageAntiBrick(int amount);

	bool IsIndestructible();
	bool IsFortified();
	void Damage(int amount);
	void Fort(int amount);

private:
	const static int MAX_HP = 100;
	const static int MAX_LIFE = 5;
	Vec3 m_size;
	CD::ShObject m_cdCube; // Derived.

	Brick_NS * m_brick_NS;
	bool m_indestructible;

	Mesh m_mesh;
	ShTex m_tex;
	ShTex m_nmap;
	ShTex m_hmap;
};

#endif
