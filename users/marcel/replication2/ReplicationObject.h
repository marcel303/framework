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
	ReplicationObject();
	virtual ~ReplicationObject();

	virtual bool RequiresUpdating() const = 0;
	virtual bool RequiresUpdate() const;

	virtual bool Serialize(BitStream & bitStream, bool init, bool send) = 0;

	//

	void SetCreationID(uint32_t id);
	uint32_t GetCreationID() const;

	void SetObjectID(uint16_t id);
	uint16_t GetObjectID() const;
};

#endif
