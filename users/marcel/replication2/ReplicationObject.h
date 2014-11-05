#ifndef REPLICATIONOBJECT_H
#define REPLICATIONOBJECT_H
#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "NetSerializable.h"
#include "Packet.h"

namespace Replication
{
	class ObjectState;

	class Object
	{
	public:
		enum SERIALIZATION_MODE
		{
			SM_CREATE,
			SM_UPDATE
		};

		Object();

		void SV_Initialize(int objectID, const std::string& className, NetSerializableObject* serializableObject);
		void CL_Initialize1(int objectID, const std::string& className);
		void CL_Initialize2(NetSerializableObject* serializableObject);

		bool Serialize(BitStream& bitStream, SERIALIZATION_MODE mode, bool send);

	private:
		bool RequireUpdating() const;
		bool RequireVersionedUpdating() const;

		// FIXME: private
	public:
		uint16_t m_objectID;
		std::string m_className;
		void* m_up;

		NetSerializableObject* m_serverSerializableObject;
		NetSerializableObject* m_clientSerializableObject;

		std::vector<int> m_clientIndicesCreate;
		std::vector<int> m_clientIndicesUpdate;
		std::vector<int> m_clientIndicesVersioned;

		bool m_serverNeedUpdate;
	};
}

#endif
