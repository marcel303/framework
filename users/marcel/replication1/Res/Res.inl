#ifndef RES_INL
#define RES_INL
#pragma once

inline const std::string& Res::GetName()
{
	return m_name;
}

inline void Res::SetType(RES_TYPE type)
{
	m_type = type;
}

#endif
