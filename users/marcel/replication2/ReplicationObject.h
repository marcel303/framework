#ifndef REPLICATIONOBJECT_H
#define REPLICATIONOBJECT_H
#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "NetSerializable.h"
#include "Packet.h"

class ReplicationObject
{
	uint16_t m_objectID;
	uint32_t m_creationID;

public:
	static const uint16_t kClassIDInvalid = -1;

	ReplicationObject()
		: m_objectID(-1)
		, m_creationID(-1)
	{
	}

	virtual ~ReplicationObject()
	{
	}

	virtual uint16_t GetClassID() const = 0;
	virtual const char * ClassName() const = 0; // GetClassName collides with a define on Window
	virtual bool RequiresUpdating() const = 0;
	virtual bool RequiresUpdate() const { return true; }

	virtual bool Serialize(BitStream & bitStream, bool init, bool send) = 0;

	//

	void SetCreationID(uint32_t id) { m_creationID = id; }
	uint32_t GetCreationID() const { return m_creationID; }

	void SetObjectID(uint16_t id) { m_objectID = id; }
	uint16_t GetObjectID() const { return m_objectID; }
};

#endif
