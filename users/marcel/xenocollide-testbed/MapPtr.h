
/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////////
// MapPtr<> is a quick and dirty smart pointer template.  MapPtr<> pointers
// utilize a reference count stored in a hash_map, indexed by pointer value.
// MapPtr<> allows any C++ object to be reference counted and deleted
// when the number of references drop to zero.

#include <map>
#include <unordered_map>

class MapPtrBase
{
protected:
	static std::unordered_map<void*, int> s_refCount;
};

template <class T>
class MapPtr : MapPtrBase
{
public:

	MapPtr()
	{
		m_pointer = NULL;
	}

	MapPtr(T* pointer)
	{
		m_pointer = pointer;
		if (pointer)
		{
			s_refCount[pointer]++;
		}
	}

	MapPtr(const MapPtr& other)
	{
		m_pointer = NULL;
		*this = other;
	}

	~MapPtr()
	{
		if ( m_pointer && --s_refCount[m_pointer] == 0 )
		{
			s_refCount.erase(m_pointer);
			delete m_pointer;
			m_pointer = NULL;
		}
	}

	void Release()
	{
		if (m_pointer)
		{
			if (--s_refCount[m_pointer] == 0)
			{
				s_refCount.erase(m_pointer);
				delete m_pointer;
				m_pointer = NULL;
			}
		}
	}

	void AddRef()
	{
		if (m_pointer)
		{
			s_refCount[m_pointer]++;
		}
	}

	MapPtr& operator= (const MapPtr& other)
	{
		Release();
		m_pointer = other.m_pointer;
		AddRef();
		return *this;
	}

	T* Pointer()
	{
		return m_pointer;
	}

	operator T*()
	{
		return m_pointer;
	}

	T* operator -> ()
	{
		return m_pointer;
	}

private:

	T* m_pointer;

};
