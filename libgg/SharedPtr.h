#pragma once

#include "Debugging.h"

template <typename T> class SharedPtr;
                      class SharedPtrRefCount;
template <typename T> class WeakPtr;

class SharedPtrRefCounter
{
public:
	int m_refCount;
	int m_refCountWeak;

	SharedPtrRefCounter()
		: m_refCount(0)
		, m_refCountWeak(0)
	{
	}

	~SharedPtrRefCounter()
	{
		Assert(m_refCount == 0);
		Assert(m_refCountWeak == 0);
	}

	bool IsExpired() const
	{
		return (m_refCount == 0);
	}

	void Acquire()
	{
		m_refCount++;

		AcquireWeak();
	}
		
	bool Release()
	{
		Assert(m_refCount > 0);

		m_refCount--;

		bool result = IsExpired();

		ReleaseWeak();

		return result;
	}

	void AcquireWeak()
	{
		m_refCountWeak++;
	}

	void ReleaseWeak()
	{
		Assert(m_refCountWeak > 0);

		m_refCountWeak--;

		if (m_refCountWeak == 0)
			delete this;
	}
};

template <typename T> class SharedPtr
{
	template <typename S> friend class SharedPtr;
	template <typename S> friend class WeakPtr;

	SharedPtrRefCounter * m_refCounter;
	T * m_value;

	void Set(SharedPtrRefCounter * refCounter, T * value)
	{
		Assert(!refCounter || !m_refCounter || value != m_value);

		if (refCounter != m_refCounter)
		{
			if (m_refCounter)
				if (m_refCounter->Release())
					delete m_value;

			m_refCounter = refCounter;

			if (m_refCounter)
				m_refCounter->Acquire();
		}

		m_value = value;
	}

	explicit SharedPtr(SharedPtrRefCounter * refCounter, T * value)
		: m_refCounter(0)
		, m_value(0)
	{
		Set(refCounter, value);
	}

public:
	SharedPtr()
		: m_refCounter(0)
		, m_value(0)
	{
	}

	template <typename S> SharedPtr(const SharedPtr<S> & other)
		: m_refCounter(0)
		, m_value(0)
	{
		*this = other;
	}

	template <typename S> explicit SharedPtr(S * object)
		: m_refCounter(0)
		, m_value(0)
	{
		*this = object;
	}

	~SharedPtr()
	{
		Set(0, 0);
	}

	template <typename S> void operator=(S * object)
	{
		if (object)
			Set(new SharedPtrRefCounter, object);
		else
			Set(0, 0);
	}

	template <typename S> void operator=(const SharedPtr<S> & ptr)
	{
		Set(ptr.m_refCounter, ptr.m_value);
	}

	T * get() const
	{
		if (m_refCounter && !m_refCounter->IsExpired())
			return m_value;
		else
			return 0;
	}

	T * operator->() const
	{
		return get();
	}

	//

	template <typename S> bool operator<(const SharedPtr<S> & other) const
	{
		return m_refCounter < other.m_refCounter;
	}
};

template <typename T> class WeakPtr
{
	SharedPtrRefCounter * m_refCounter;
	T * m_value;

	void Set(SharedPtrRefCounter * refCounter, T * value)
	{
		Assert(!refCounter || !m_refCounter || value != m_value);

		if (refCounter != m_refCounter)
		{
			if (m_refCounter)
				m_refCounter->ReleaseWeak();

			m_refCounter = refCounter;

			if (m_refCounter)
				m_refCounter->AcquireWeak();
		}

		m_value = value;
	}

public:
	WeakPtr()
		: m_refCounter(0)
		, m_value(0)
	{
	}

	template <typename S> WeakPtr(SharedPtr<S> & ptr)
		: m_refCounter(0)
		, m_value(0)
	{
		*this = ptr;
	}

	~WeakPtr()
	{
		Set(0, 0);
	}

	template <typename S> WeakPtr<T> & operator=(SharedPtr<S> & ptr)
	{
		Set(ptr.m_refCounter, ptr.m_value);

		return *this;
	}

	bool expired() const
	{
		return m_refCounter->IsExpired();
	}

	SharedPtr<T> lock()
	{
		Assert(!expired());

		return SharedPtr<T>(m_refCounter, m_value);
	}

	template <typename S> void operator=(const SharedPtr<S> & ptr)
	{
		Set(ptr.m_refCounter, ptr.m_value);
	}

	//

	template <typename S> bool operator<(const WeakPtr<S> & other) const
	{
		return m_refCounter < other.m_refCounter;
	}
};
