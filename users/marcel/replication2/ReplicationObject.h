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
	// todo : add replication object interface. game must inherit. add objects to replication mgr that are already an object.
	//        removes the additional overhead of creating separate objects. move away from storing NetSerializableObject here

	class IObject // todo : rename, once Object itself has been renamed
	{
		uint16_t m_objectID;

	public:
		static const uint16_t kClassIDInvalid = -1;

		IObject()
			: m_objectID(-1)
		{
		}

		virtual ~IObject()
		{
		}

		virtual uint16_t GetClassID() const = 0;
		virtual const char * ClassName() const = 0; // GetClassName collides with a define on Window
		virtual bool RequiresUpdating() const = 0;
		virtual bool RequiresUpdate() const { return true; }

		virtual bool Serialize(BitStream & bitStream, bool init, bool send) = 0;

		void SetObjectID(uint16_t id) { m_objectID = id; }
		uint16_t GetObjectID() const { return m_objectID; }
	};

	class Object : public IObject // todo : remove Object class eventually
	{
	public:
		Object();

		void SV_Initialize(int objectID, int creationID, const std::string & className, IObject * object);
		void CL_Initialize1(int objectID, const std::string & className);
		void CL_Initialize2(IObject * object);

		virtual uint16_t GetClassID() const
		{
			return kClassIDInvalid;
		}

		virtual const char * ClassName() const
		{
			return m_className.c_str();
		}

		virtual bool RequiresUpdating() const
		{
			return m_serverNeedUpdate;
		}

		virtual bool RequiresUpdate() const
		{
			return true;
		}

		virtual bool Serialize(BitStream & bitStream, bool init, bool send);

	private:
		bool RequireUpdating() const;

		// FIXME: private
	public:
		std::string m_className;
		void * m_up;

		IObject * m_serverObject;
		IObject * m_clientObject;

		bool m_serverNeedUpdate;
		int m_serverObjectCreationId;
	};
}

#endif
