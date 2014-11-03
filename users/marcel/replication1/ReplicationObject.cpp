#include "BitStream.h"
#include "Debug.h"
#include "ReplicationObject.h"
#include "ReplicationObjectState.h"
#include "Types.h"

#define INDEX_CREATE_ONLY -1
#define INDEX_SKIP -2

namespace Replication
{
	Object::Object()
	{
		m_objectID = 0;
		m_up = 0;
		m_serverParameters = 0;
		m_clientParameters = 0;
		m_serverNeedUpdate = false;
		m_serverNeedVersionUpdate = false;
		m_serverPriority = 0;
	}

	void Object::SV_Initialize(int objectID, const std::string& className, ParameterList* parameters, Priority* priority)
	{
		FASSERT(parameters);
		FASSERT(priority);

		m_objectID = objectID;
		m_className = className;
		m_serverParameters = parameters;
		m_serverNeedUpdate = RequireUpdating();
		m_serverNeedVersionUpdate = RequireVersionedUpdating();
		m_serverPriority = priority;
	}

	void Object::CL_Initialize1(int objectID, const std::string& className)
	{
		m_objectID = objectID;
		m_className = className;
	}

	void Object::CL_Initialize2(ParameterList* parameters)
	{
		m_clientParameters = parameters;
	}

	bool Object::SV_Serialize(BitStream& bitStream, SERIALIZATION_MODE mode, ObjectState& state) const
	{
		if (!m_serverParameters)
			return false;

		if (mode == SM_VERSION)
		{
			uint8_t parameterCount = state.SV_RequireVersionedUpdate();

			bitStream.Write(parameterCount);
		}

		// Loop through server parameters, fill.
		for (size_t i = 0; i < m_serverParameters->m_parameters.size(); ++i)
		{
			const Parameter* parameter = m_serverParameters->m_parameters[i].get();

			if (mode == SM_UPDATE && parameter->m_rep != REP_CONTINUOUS)
				continue;
			if (mode == SM_VERSION && parameter->m_rep != REP_ONCHANGE)
				continue;
			if (mode == SM_VERSION && parameter->m_version == state.m_versioning[i])
				continue;
			
			if (mode == SM_VERSION)
			{
				uint8_t parameterIndex = (uint8_t)i;
				bitStream.Write(parameterIndex);

				state.m_versioning[i] = parameter->m_version;
			}

			if (!parameter->RepSerialize(bitStream))
				return false;
		}

		return true;
	}

	bool Object::CL_DeSerialize(BitStream& bitStream, SERIALIZATION_MODE mode)
	{
		// Loop through client indices. If server parameter present on client, fill.
		std::vector<int>* _indices = 0;
		
		if (mode == SM_UPDATE)
			_indices = &m_clientIndicesUpdate;
		if (mode == SM_CREATE)
			_indices = &m_clientIndicesCreate;
		if (mode == SM_VERSION)
			_indices = &m_clientIndicesVersioned;

		std::vector<int>& indices = *_indices;

		if (mode == SM_UPDATE || mode == SM_CREATE)
		{
			for (size_t i = 0; i < indices.size(); ++i)
			{
				if (mode == SM_UPDATE && indices[i] == INDEX_CREATE_ONLY)
					continue;

				if (indices[i] != INDEX_SKIP)
				{
					Parameter* parameter = m_clientParameters->m_parameters[indices[i]].get();

					if (!parameter->RepDeSerialize(bitStream))
 						return false;
				}
				else
				{
					Parameter* parameter = m_serverParameters->m_parameters[i].get();

					int size = parameter->GetRepParameterSize();

					bitStream.ReadSkip(size << 3);
				}
			}
		}
		else if (mode == SM_VERSION)
		{
			uint8_t parameterCount;

			bitStream.Read(parameterCount);

			for (size_t p = 0; p < parameterCount; ++p)
			{
				uint8_t i;

				bitStream.Read(i);

				if (i >= indices.size())
				{
					DB_ERR("WTF\n");
					continue;
				}

				if (indices[i] != INDEX_CREATE_ONLY)
				{
					if (indices[i] != INDEX_SKIP)
					{
						Parameter* parameter = m_clientParameters->m_parameters[indices[i]].get();

						if (!parameter->RepDeSerialize(bitStream))
							return false;
					}
					else
					{
						Parameter* parameter = m_serverParameters->m_parameters[i].get();

						int size = parameter->GetRepParameterSize();

						bitStream.ReadSkip(size << 3);
					}
				}
			}
		}

		return true;
	}

	bool Object::SV_SerializeStructure(BitStream& bitStream) const
	{
		uint8_t parameterCount = (uint8_t)m_serverParameters->m_parameters.size();

		bitStream.Write(parameterCount);

		for (uint32_t i = 0; i < parameterCount; ++i)
		{
			const Parameter* parameter = m_serverParameters->m_parameters[i].get();

			const uint8_t rep = parameter->m_rep;
			const uint8_t type = parameter->m_type;
			const std::string& name = parameter->m_name;
			const uint8_t nameLength = (uint8_t)name.size();

			bitStream.Write(rep);
			bitStream.Write(type);
			bitStream.Write(nameLength);

			for (uint32_t j = 0; j < nameLength; ++j)
			{
				uint8_t c = name[j];
				bitStream.Write(c);
			}
		}

		return true;
	}

	bool Object::CL_DeSerializeStructure(BitStream& bitStream)
	{
		// Read structure from packet.
		m_serverParameters = new ParameterList();

		uint8_t parameterCount;

		bitStream.Read(parameterCount);

		//printf("RepCreate: Structure.\n");
		//printf("\tParameterCount: %d.\n", parameterCount);

		for (uint32_t i = 0; i < parameterCount; ++i)
		{
			Parameter parameter;

			uint8_t rep;
			uint8_t type;
			std::string name;
			uint8_t nameLength;

			bitStream.Read(rep);
			bitStream.Read(type);
			bitStream.Read(nameLength);

			for (uint32_t j = 0; j < nameLength; ++j)
			{
				uint8_t c;
				bitStream.Read(c);
				name.push_back(c);
			}

			parameter.Initialize((PARAMETER_TYPE)type, name, (REPLICATION_MODE)rep, COMPRESS_NONE, 0);
			m_serverParameters->Add(parameter);

			//printf("\tParameter: %d, %s.\n", type, name.c_str());
		}

		// Calculate indices.
		// (CREATE)
		for (size_t i = 0; i < m_serverParameters->m_parameters.size(); ++i)
		{
			int index = INDEX_SKIP;

			for (size_t j = 0; j < m_clientParameters->m_parameters.size(); ++j)
			{
				if (m_serverParameters->m_parameters[i]->m_type == m_clientParameters->m_parameters[j]->m_type &&
					m_serverParameters->m_parameters[i]->m_name == m_clientParameters->m_parameters[j]->m_name)
				{
					index = (int)j;
				}
			}

			//printf("\t\tMapped input %d to output %d.\n", i, index);

			m_clientIndicesCreate.push_back(index);
		}

		// (UPDATE)
		for (size_t i = 0; i < m_serverParameters->m_parameters.size(); ++i)
		{
			int index = INDEX_SKIP;

			for (size_t j = 0; j < m_clientParameters->m_parameters.size(); ++j)
			{
				if (m_serverParameters->m_parameters[i]->m_type == m_clientParameters->m_parameters[j]->m_type &&
					m_serverParameters->m_parameters[i]->m_name == m_clientParameters->m_parameters[j]->m_name)
				{
					if (m_serverParameters->m_parameters[i]->m_rep == REP_CONTINUOUS)
						index = (int)j;
					else
						index = INDEX_CREATE_ONLY;
				}
			}

			//printf("\t\tMapped input %d to output %d.\n", i, index);

			m_clientIndicesUpdate.push_back(index);
		}

		// (VERSIONED)
		for (size_t i = 0; i < m_serverParameters->m_parameters.size(); ++i)
		{
			int index = INDEX_SKIP;

			for (size_t j = 0; j < m_clientParameters->m_parameters.size(); ++j)
			{
				if (m_serverParameters->m_parameters[i]->m_type == m_clientParameters->m_parameters[j]->m_type &&
					m_serverParameters->m_parameters[i]->m_name == m_clientParameters->m_parameters[j]->m_name)
				{
					if (m_serverParameters->m_parameters[i]->m_rep == REP_ONCHANGE)
						index = (int)j;
					else
						index = INDEX_CREATE_ONLY;
				}
			}

			//printf("\t\tMapped input %d to output %d.\n", i, index);

			m_clientIndicesVersioned.push_back(index);
		}

		return true;
	}

	bool Object::RequireUpdating() const
	{
		if (!m_serverParameters)
			return false;

		for (size_t i = 0; i < m_serverParameters->m_parameters.size(); ++i)
			if (m_serverParameters->m_parameters[i]->m_rep == REP_CONTINUOUS)
				return true;

		return false;
	}

	bool Object::RequireVersionedUpdating() const
	{
		if (!m_serverParameters)
			return false;

		for (size_t i = 0; i < m_serverParameters->m_parameters.size(); ++i)
			if (m_serverParameters->m_parameters[i]->m_rep == REP_ONCHANGE)
				return true;

		return false;
	}
}
