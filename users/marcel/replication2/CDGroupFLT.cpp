#include "CDGroupFLT.h"

namespace CD
{
	// GroupFlt
	GroupFlt::~GroupFlt()
	{
	}

	// GroupFltACL
	GroupFltACL::GroupFltACL()
		: m_deny(0)
		, m_permit(0)
		, m_default(true)
	{
	}

	bool GroupFltACL::Accept(Group group, Object* object)
	{
		Assert(group < kMaxGroups);

		if ((m_deny & (1 << group)) != 0)
			return false;

		if ((m_permit & (1 << group)) != 0)
			return true;

		return m_default;
	}

	void GroupFltACL::Permit(Group group)
	{
		Assert(group < kMaxGroups);

		m_permit |= (1 << group);
	}

	void GroupFltACL::Deny(Group group)
	{
		Assert(group < kMaxGroups);

		m_deny |= (1 << group);
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
