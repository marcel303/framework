#pragma once

#include "AnimationTimer.h"
#include "SelectionBuffer.h"
#include "SelectionMap.h"
#include "Shape_Polygon.h"
#include "SpriteMgr.h"

namespace Xi
{
	class Link;

	enum PartType
	{
		PartType_Body,
		PartType_Weapon,
		PartType_Thruster
	};

	enum WeaponType
	{
		WeaponType_Vulcan,  // Rapidly firing bullets.
		WeaponType_Missile, // Slower moving, smart aim missiles.
		WeaponType_Gas,     // Slow moving large area poisenous gasses.
		WeaponType_Laser   // Laser beams.
	};

	class Part
	{
	public:
		Part();
		~Part();

		Part* Copy();

		void Update();
		void UpdateTransform();
		void RenderSB(SelectionBuffer* sb);
		void Render(BITMAP* buffer);
		void Hit();

		PartType PartType;
		float RotationSpeed;
		Shape_Polygon Shape;
		BoundingSphere BoundingShape;
		std::vector<Link*> Links;
		int SelectionIndex;
		int HitPoints;
		AnimationTimer HitTimer;
		BOOL IsAlive;
		ISprite* Sprite;
	};
};
