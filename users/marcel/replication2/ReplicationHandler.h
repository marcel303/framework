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
		virtual bool OnReplicationObjectCreate1(Client * client, const std::string & className, Object ** out_object) = 0;
		virtual void OnReplicationObjectCreate2(Client * client, Replication::Object* object) = 0;
		virtual void OnReplicationObjectDestroy(Client * client, Replication::Object* object) = 0;
	};
}

#endif
