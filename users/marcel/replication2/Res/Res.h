#ifndef RES_H
#define RES_H
#pragma once

#include <string>
#include <vector>
#include "Debug.h"
#include "ResUser.h"
#include "SharedPtr.h"
#include "Types.h"

enum RES_TYPE
{
	RES_NONE,
	RES_BASE_TEX,
	RES_FONT,
	RES_IB,
	RES_PS,
	RES_SHADER,
	RES_SND,
	RES_SND_SRC,
	RES_TEX,
	RES_TEXCR,
	RES_TEXCD,
	RES_TEXCF,
	RES_TEXD,
	RES_TEXR,
	RES_TEXRECTR,
	RES_VB,
	RES_VS,
	RES_COUNT // Number of resource types.
};

class Res
{
public:
	Res();
	virtual ~Res();

	inline const std::string& GetName();

	inline void SetType(RES_TYPE type);

	void Invalidate();
	void AddUser(ResUser* user);
	void RemoveUser(ResUser* user);

	RES_TYPE m_type;
	std::string m_name;
	std::string m_filename;
	ResUser* m_users[4];
	uint32_t m_userCount;
	uint32_t m_version;
};

#include "Res.inl"

typedef SharedPtr<Res> ShBaseRes;

class ResPtrGeneric
{
public:
	inline ResPtrGeneric()
	{
	}

	inline Res* get() const
	{
		return m_res.get();
	}

	ShBaseRes m_res;
};

typedef SharedPtr<ResPtrGeneric> ShBaseResPtr;
typedef WeakPtr<ResPtrGeneric> RefBaseResPtr;

template <class T>
class ResPtr
{
public:
	inline ResPtr()
	{
	}

	inline ResPtr(Res* res)
	{
		Assert(res);

		ResPtrGeneric* ptr = new ResPtrGeneric();

		ptr->m_res = ShBaseRes(res);

		m_ptr = ShBaseResPtr(ptr);
	}

	inline ResPtr(const ShBaseResPtr& ptr)
	{
		Assert(ptr.get());

		m_ptr = ptr;
	}

	inline T* get() const
	{
		if (m_ptr.get() == 0)
			return 0;
		else
			return static_cast<T*>(m_ptr.get()->get());
	}

	inline T* operator->() const
	{
		return get();
	}

	inline operator ShBaseResPtr()
	{
		return m_ptr;
	}

	ShBaseResPtr m_ptr;
};

#include "ResTypes.h"

#endif
