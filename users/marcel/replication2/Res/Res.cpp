#include "Res.h"

Res::Res()
	: m_userCount(0)
{
	SetType(RES_NONE);

	memset(m_users, 0, sizeof(m_users)); // todo : debug only ?

	m_version = 0;
}

Res::~Res()
{
	for (uint32_t i = 0; i < m_userCount; ++i)
		m_users[i]->OnResDestroy(this);

	FASSERT(m_userCount == 0);
}

void Res::Invalidate()
{
	m_version++;

	for (uint32_t i = 0; i < m_userCount; ++i)
		m_users[i]->OnResInvalidate(this);
}

void Res::AddUser(ResUser* user)
{
	FASSERT(m_userCount < 4);

	if (m_userCount == 4)
		return;

	m_users[m_userCount++] = user;
}

void Res::RemoveUser(ResUser* user)
{
	uint32_t i = 0;
	
	for (; i < 4; ++i)
		if (m_users[i] == user)
			break;

	if (i == 4)
		return;

	for (; i + 1 < 4; ++i)
		m_users[i] = m_users[i + 1];

	m_userCount--;
}
