#ifndef REPLICATIONOBJECT_H
#define REPLICATIONOBJECT_H
#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "Packet.h"
#include "ParameterList.h"
#include "ReplicationPriority.h"

namespace Replication
{
	class ObjectState;

	class Object
	{
	public:
		enum SERIALIZATION_MODE
		{
			SM_CREATE,
			SM_UPDATE,
			SM_VERSION
		};

		Object();

		void SV_Initialize(int objectID, const std::string& className, ParameterList* parameters, Priority* priority);
		void CL_Initialize1(int objectID, const std::string& className);
		void CL_Initialize2(ParameterList* parameters);

		bool SV_Serialize(BitStream& bitStream, SERIALIZATION_MODE mode, ObjectState& state) const;
		bool CL_DeSerialize(BitStream& bitStream, SERIALIZATION_MODE mode);
		bool SV_SerializeStructure(BitStream& bitStream) const;
		bool CL_DeSerializeStructure(BitStream& bitStream);

	private:
		bool RequireUpdating() const;
		bool RequireVersionedUpdating() const;

		// FIXME: private
	public:
		uint16_t m_objectID;
		std::string m_className;
		void* m_up;
		ParameterList* m_serverParameters;
		ParameterList* m_clientParameters;
		std::vector<int> m_clientIndicesCreate;
		std::vector<int> m_clientIndicesUpdate;
		std::vector<int> m_clientIndicesVersioned;
		bool m_serverNeedUpdate;
		bool m_serverNeedVersionUpdate;
		Priority* m_serverPriority;
	};
}

#endif
