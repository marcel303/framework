#ifndef REPLICATIONHANDLER_H
#define REPLICATIONHANDLER_H
#pragma once

#include <string>
#include "ReplicationClient.h"

class ReplicationHandler
{
public:
	virtual bool OnReplicationObjectSerializeType(ReplicationClient * client, ReplicationObject * object, BitStream & bitStream) = 0;
	virtual bool OnReplicationObjectCreateType(ReplicationClient * client, BitStream & className, ReplicationObject ** out_object) = 0;
	virtual void OnReplicationObjectCreated(ReplicationClient * client, ReplicationObject * object) = 0;
	virtual void OnReplicationObjectDestroyed(ReplicationClient * client, ReplicationObject * object) = 0;
};

#endif
