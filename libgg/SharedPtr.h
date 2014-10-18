#pragma once

#include "Debugging.h"

template <typename T>
class SharedPtr
{
	class Storage
	{
	public:
		T * m_object;
		int m_refCount;

		Storage(T * object)
			: m_object(object)
			, m_refCount(0)
		{
			Assert(object);
		}

		~Storage()
		{
			Assert(m_refCount == 0);

			delete m_object;
		}

		void Acquire()
		{
			m_refCount++;
		}
		
		void Release()
		{
			Assert(m_refCount > 0);

			m_refCount--;

			if (m_refCount == 0)
				delete this;
		}
	};

	Storage * m_storage;

	void SetStorage(Storage * storage)
	{
		Assert(!storage || !m_storage || storage->m_object != m_storage->m_object);

		if (storage != m_storage)
		{
			if (m_storage)
				m_storage->Release();
			m_storage = storage;
			if (m_storage)
				m_storage->Acquire();
		}
	}

public:
	SharedPtr()
		: m_storage(0)
	{
	}

	SharedPtr(SharedPtr & other)
		: m_storage(0)
	{
		SetStorage(other.m_storage);
	}

	SharedPtr(T * object)
		: m_storage(0)
	{
		*this = object;
	}

	~SharedPtr()
	{
		if (m_storage)
			m_storage->Release();
	}

	void operator=(T * object)
	{
		if (object)
			SetStorage(new Storage(object));
		else
			SetStorage(0);
	}

	void operator=(SharedPtr & ptr)
	{
		SetStorage(ptr.m_storage);
	}

	T * get()
	{
		if (m_storage)
			return m_storage->m_object;
		else
			return 0;
	}

	const T * get() const
	{
		if (m_storage)
			return m_storage->m_object;
		else
			return 0;
	}

	T * operator->()
	{
		return get();
	}
};
