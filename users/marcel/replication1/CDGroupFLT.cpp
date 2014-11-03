#include "CDGroupFLT.h"

namespace CD
{
	// GroupFlt
	GroupFlt::~GroupFlt()
	{
	}

	// GroupFltACL
	bool GroupFltACL::Accept(Group group, Object* object)
	{
		if (m_deny.count(group) != 0)
			return false;

		if (m_permit.count(group) != 0)
			return true;

		return m_default;
	}

	void GroupFltACL::Permit(Group group)
	{
		m_permit[group] = 1;
	}

	void GroupFltACL::Deny(Group group)
	{
		m_deny[group] = 1;
	}

	void GroupFltACL::Default(bool permit)
	{
		m_default = permit;
	}

	// GroupFltEX
	bool GroupFltEX::Accept(Group group, Object* object)
	{
		if (m_denyObjects.count(object) != 0)
			return false;

		return GroupFltACL::Accept(group, object);
	}

	void GroupFltEX::DenyObject(Object* object)
	{
		m_denyObjects[object] = 1;
	}
}
