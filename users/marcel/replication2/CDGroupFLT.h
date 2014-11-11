#ifndef CDGROUPFLT_H
#define CDGROUPFLT_H
#pragma once

#include <map>
#include <stdint.h>
#include "CDObject.h"

namespace CD
{
	typedef int Group;

	class GroupFlt
	{
	public:
		virtual ~GroupFlt();
		virtual bool Accept(Group group, Object* object) = 0;
	};

	class GroupFltACL : public GroupFlt
	{
	public:
		GroupFltACL();

		virtual bool Accept(Group group, Object* object);

		void Permit(Group group);
		void Deny(Group group);
		void Default(bool permit);

	private:
		const static int kMaxGroups = 32;

		uint32_t m_permit;
		uint32_t m_deny;
		bool m_default;
	};

	class GroupFltEX : public GroupFltACL
	{
	public:
		virtual bool Accept(Group group, Object* object);

		void DenyObject(Object* object);

	private:
		std::map<Object*, int> m_denyObjects;
	};
}

#endif
