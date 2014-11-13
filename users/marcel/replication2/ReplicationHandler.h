#ifndef REPLICATIONHANDLER_H
#define REPLICATIONHANDLER_H
#pragma once

#include <string>
#include "ReplicationClient.h"

class ReplicationHandler
{
public:
	virtual bool OnReplicationObjectCreate1(ReplicationClient * client, const std::string & className, ReplicationObject ** out_object) = 0;
	virtual void OnReplicationObjectCreate2(ReplicationClient * client, ReplicationObject * object) = 0;
	virtual void OnReplicationObjectDestroy(ReplicationClient * client, ReplicationObject * object) = 0;
};

#endif
