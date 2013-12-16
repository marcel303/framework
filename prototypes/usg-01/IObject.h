#pragma once

#include <allegro.h>
#include <algorithm>
#include <vector>
#include "SelectionBuffer.h"

class Map;

enum ObjectType
{
	ObjectType_Sun,
	ObjectType_Gun,
	ObjectType_GunBullet,
	ObjectType_Ship,
	ObjectType_BlackHole,
	ObjectType_Boss
};

enum ObjectFlag
{
	ObjectFlag_Dead = 0x1
};

enum ObjectMessage
{
	//ObjectMessage_DidCreate,
	ObjectMessage_HasDied
};

class ObjectEvent
{
public:
	void* Data;
};

class Map;

class IObject
{
public:
	IObject(ObjectType type, Map* map)
	{
		m_ObjectType = type;
		m_ObjectFlags = 0;
		m_Map = map;
	}

	virtual void Update(Map* map) = 0;
	virtual void Render(BITMAP* buffer) = 0;
	virtual void RenderSB(SelectionBuffer* sb)
	{
	}
	virtual void HandleMessage(ObjectMessage message)
	{
	}

	void SetFlag(ObjectFlag flag)
	{
		m_ObjectFlags |= flag;

		if (flag == ObjectFlag_Dead)
		{
			HandleMessage(ObjectMessage_HasDied);
		}
	}

	ObjectType m_ObjectType;
	Map* m_Map;
	int m_ObjectFlags;
};
