#include "BitStream.h"
#include "Debug.h"
#include "ReplicationObject.h"
#include "Types.h"

ReplicationObject::ReplicationObject()
	: m_objectID(-1)
	, m_creationID(-1)
{
}

ReplicationObject::~ReplicationObject()
{
}

bool ReplicationObject::RequiresUpdate() const
{
	return true;
}

//

void ReplicationObject::SetCreationID(uint32_t id)
{
	m_creationID = id;
}

uint32_t ReplicationObject::GetCreationID() const
{
	return m_creationID;
}

void ReplicationObject::SetObjectID(uint16_t id)
{
	m_objectID = id;
}

uint16_t ReplicationObject::GetObjectID() const
{
	return m_objectID;
}
