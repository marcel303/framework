#ifndef REPLICATIONHANDLER_H
#define REPLICATIONHANDLER_H
#pragma once

#include <string>
#include "ReplicationClient.h"

namespace Replication
{
	class Handler
	{
	public:
		virtual bool OnReplicationObjectCreate1(Client * client, const std::string & className, NetSerializableObject ** out_serializableObject, void ** out_up) = 0;
		virtual void OnReplicationObjectCreate2(Client * client, void * up) = 0;
		virtual void OnReplicationObjectDestroy(Client * client, void * up) = 0;
	};
}

#endif
