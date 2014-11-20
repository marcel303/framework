#ifndef REPLICATIONHANDLER_H
#define REPLICATIONHANDLER_H
#pragma once

//#include "libnet_forward.h"

class BitStream;
class ReplicationClient;
class ReplicationObject;

class ReplicationHandler
{
public:
	virtual bool OnReplicationObjectSerializeType(ReplicationClient * client, ReplicationObject * object, BitStream & bitStream) = 0;
	virtual bool OnReplicationObjectCreateType(ReplicationClient * client, BitStream & bitStream, ReplicationObject ** out_object) = 0;
	virtual void OnReplicationObjectCreated(ReplicationClient * client, ReplicationObject * object) = 0;
	virtual void OnReplicationObjectDestroyed(ReplicationClient * client, ReplicationObject * object) = 0;
};

#endif
