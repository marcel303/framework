#pragma once

#include <stdint.h>
#include <xmmintrin.h>
#include "Debugging.h"
#include "FastList.h"
#include "MemOps.h"

class IMemAllocator;
class MemAllocatorGeneric;      // layers on top of standard library functions to provide 16 byte aligned allocations
class MemAllocatorArray;        // allocates ascending addresses from a custom provided array of data
class MemAllocatorTransient;    // layers on top of MemAllocatorArray. manages allocation of the array data
class MemAllocatorStack;        // layers on top of MemAllocatorArray. provides a basic Free implementation. the freed data must always live at the end
class MemAllocatorManualStack;  // layers on top of MemAllocatorArray. the current allocation cursor can be pushed and popped manually
template <uint32_t N, uint32_t POW2>
class MemAllocatorFixed;        // fixed block allocator using a free list

class IMemAllocationReporter;   // debugging layer
class MemAllocInfo;
class MemAllocInfoListIterator;

typedef FastList<MemAllocInfo> MemAllocInfoList;

class IMemAllocator
{
public:
	virtual void * Alloc(size_t size, uint32_t tag = 0) = 0;
	virtual void Free(void * p) = 0;

	// new & delete

	template <typename T>
	inline T * New()
	{
		void * pBytes = Alloc(sizeof(T));
		return new(pBytes) T();
	}

	template <typename T>
	inline T * New(uint32_t num)
	{
		void * pBytes = Alloc(sizeof(T) * num);
		return new(pBytes) T[num];
	}

	template <typename T>
	inline void Delete(T * p)
	{
		p->~T();
		Free(p);
	}

	template <typename T>
	inline void SafeDelete(T * & p)
	{
		Delete(p);
		p = 0;
	}

	template <typename T>
	inline void SafeFree(T * & p)
	{
		if (p != 0)
		{
			Free(p);
			p = 0;
		}
	}
};

class IMemAllocationIterator
{
public:
	virtual bool HasValue() = 0;
	virtual void Next() = 0;
	virtual MemAllocInfo * Value() = 0;
};

class IMemAllocationReporter
{
public:
	virtual IMemAllocationIterator * GetAllocationIterator() = 0;
};

ALIGN_CLASS(16) MemAllocInfo
{
public:
	inline MemAllocInfo()
		: m_pPrev(0)
		, m_pNext(0)
		, m_size(0)
	{
	}

	inline void Init(uint32_t size, uint32_t tag, void * pAddress)
	{
		m_pPrev = m_pNext = 0;
		m_size = size >> 4;
		m_tag = tag;
		m_address = pAddress;
	}

	inline uint32_t GetSize() const
	{
		return m_size << 4;
	}

	inline uint32_t GetTag() const
	{
		return m_tag;
	}

	inline void * GetAddress() const
	{
		return reinterpret_cast<void *>(m_address);
	}

	MemAllocInfo * m_pPrev;
	MemAllocInfo * m_pNext;
	uint32_t m_size : 24;
	uint32_t m_tag : 8;
	void * m_address;
};

class MemAllocInfoListIterator : public IMemAllocationIterator
{
public:
	MemAllocInfoListIterator()
		: m_pAllocInfo(0)
	{
	}

	virtual bool HasValue()
	{
		return m_pAllocInfo != 0;
	}

	virtual void Next()
	{
		m_pAllocInfo = m_pAllocInfo->m_pNext;
	}

	virtual MemAllocInfo * Value()
	{
		return m_pAllocInfo;
	}

	void Begin(MemAllocInfoList & pList)
	{
		m_pAllocInfo = pList.head();
	}

private:
	MemAllocInfo * m_pAllocInfo;
};

class MemAllocatorGeneric
	: public IMemAllocator
	, public IMemAllocationReporter
{
public:
	MemAllocatorGeneric(uint32_t alignment)
		: m_alignment(alignment)
	{
	}

	virtual ~MemAllocatorGeneric()
	{
		Assert(m_allocList.size() == 0);
	}
	
	virtual void * Alloc(size_t size, uint32_t tag = 0)
	{
#ifdef _DEBUG
		void * p = _mm_malloc(size + sizeof(MemAllocInfo), m_alignment);

		MemAllocInfo * pAllocInfo = reinterpret_cast<MemAllocInfo *>(p);
		pAllocInfo->Init(size, tag, 0);

		m_allocList.push_tail(pAllocInfo);

		return pAllocInfo + 1;
#else
		return _mm_malloc(size, m_alignment);
#endif
	}

	virtual void Free(void * p)
	{
#ifdef _DEBUG
		MemAllocInfo * pAllocInfo = reinterpret_cast<MemAllocInfo *>(p) - 1;

		m_allocList.erase(pAllocInfo);

		_mm_free(pAllocInfo);
#else
		_mm_free(p);
#endif
	}

	virtual IMemAllocationIterator * GetAllocationIterator()
	{
		m_iterator.Begin(m_allocList);
		
		return &m_iterator;
	}

private:
	uint32_t m_alignment;
	MemAllocInfoList m_allocList;
	MemAllocInfoListIterator m_iterator;
};

class MemAllocatorArray
	: public IMemAllocator
	, public IMemAllocationReporter
{
public:
	MemAllocatorArray(void * pBytes, size_t size)
		: m_pBytes(0)
		, m_pBytePtr(0)
		, m_size(0)
	{
		SetBytes(pBytes, size);
	}

	virtual void * Alloc(size_t size, uint32_t tag = 0)
	{
		size = (size + 15) & ~0xf;

#ifdef _DEBUG
		MemAllocInfo * pAllocInfo = reinterpret_cast<MemAllocInfo *>(m_pBytePtr);
		pAllocInfo->Init(size, tag, 0);

		m_pBytePtr += sizeof(MemAllocInfo) + size;

		Assert(m_pBytePtr <= m_pBytes + m_size);

		m_allocList.push_tail(pAllocInfo);

		return pAllocInfo + 1;
#else
		void * pResult = m_pBytePtr;

		m_pBytePtr += size;

		return pResult;
#endif
	}

	virtual void Free(void * p)
	{
		//Assert(false);
	}

	virtual IMemAllocationIterator * GetAllocationIterator()
	{
		m_iterator.Begin(m_allocList);

		return &m_iterator;
	}

	void Reset()
	{
		m_allocList.discard();

		m_pBytePtr = m_pBytes;
	}

protected:
	inline void * GetBytes()
	{
		return m_pBytes;
	}

	void SetBytes(void * p, uint32_t size)
	{
		Assert(m_pBytes == m_pBytePtr);

		m_pBytes = reinterpret_cast<uint8_t *>(p);
		m_pBytePtr = m_pBytes;
		m_size = size;
	}

	void * GetBytePtr()
	{
		return m_pBytePtr;
	}

	void SetBytePtr(void * p)
	{
		m_pBytePtr = reinterpret_cast<uint8_t *>(p);
	}

	inline void PopAllocTillBytePtr(void * pBytePtr)
	{
#ifdef _DEBUG
		while (m_allocList.tail() != 0 && m_allocList.tail() < pBytePtr)
			m_allocList.pop_tail();
#endif
	}

	inline void PopAllocAndSetBytePtr(void * pBytePtr)
	{
#ifdef _DEBUG
		MemAllocInfo * pAllocInfo = reinterpret_cast<MemAllocInfo *>(pBytePtr) - 1;
		MemAllocInfo * pTail = m_allocList.pop_tail_value();
		Assert(pTail == pAllocInfo);
		pBytePtr = pAllocInfo;
#endif

		m_pBytePtr = reinterpret_cast<uint8_t *>(pBytePtr);
	}

private:
	uint8_t * m_pBytes;
	uint8_t * m_pBytePtr;
	size_t m_size;
	MemAllocInfoList m_allocList;
	MemAllocInfoListIterator m_iterator;
};

class MemAllocatorTransient : public MemAllocatorArray
{
public:
	MemAllocatorTransient(size_t size)
		: MemAllocatorArray(_mm_malloc(size, 16), size)
	{
	}

	virtual ~MemAllocatorTransient()
	{
		void * pBytes = GetBytes();
		_mm_free(pBytes);
		SetBytes(0, 0);
	}
};

class MemAllocatorStack : public MemAllocatorArray
{
public:
	MemAllocatorStack(uint32_t size, uint32_t maxStackSize)
		: MemAllocatorArray(_mm_malloc(size, 16), size)
		, m_pStack(new void*[maxStackSize])
		, m_stackSize(0)
#ifdef DEBUG
		, m_maxStackSize(maxStackSize)
#endif
	{
	}

	virtual ~MemAllocatorStack()
	{
		Assert(m_stackSize == 0);

		void * pBytes = GetBytes();
		_mm_free(pBytes);
		SetBytes(0, 0);

		delete[] m_pStack;
		m_pStack = 0;
	}

	virtual void * Alloc(size_t size, uint32_t tag = 0)
	{
		Assert(m_stackSize <= m_maxStackSize);
#ifdef DEBUG
		if (m_stackSize == m_maxStackSize)
			return 0;
#endif
		void * p = MemAllocatorArray::Alloc(size, tag);
		m_pStack[m_stackSize] = p;
		m_stackSize++;
		return p;
	}

	virtual void Free(void * p)
	{
		Assert(m_stackSize != 0 && m_pStack[m_stackSize - 1] == p);
		PopAllocAndSetBytePtr(m_pStack[m_stackSize - 1]);
		m_stackSize--;
	}

private:
	void * * m_pStack;
	uint32_t m_stackSize;
#ifdef DEBUG
	uint32_t m_maxStackSize;
#endif
};

class MemAllocatorManualStack : public MemAllocatorArray
{
public:
	MemAllocatorManualStack(uint32_t size, uint32_t maxStackSize)
		: MemAllocatorArray(_mm_malloc(size, 16), size)
		, m_pStack(new void*[maxStackSize])
		, m_stackSize(0)
#ifdef DEBUG
		, m_maxStackSize(maxStackSize)
#endif
	{
	}

	virtual ~MemAllocatorManualStack()
	{
		Assert(m_stackSize == 0);

		void * pBytes = GetBytes();
		_mm_free(pBytes);
		SetBytes(0, 0);

		delete[] m_pStack;
		m_pStack = 0;
	}

	virtual void * Alloc(size_t size, uint32_t tag = 0)
	{
		return MemAllocatorArray::Alloc(size, tag);
	}

	virtual void Free(void * p)
	{
	}

	inline void Push()
	{
		Assert(m_stackSize <= m_maxStackSize);
#ifdef _DEBUG
		if (m_stackSize == m_maxStackSize)
			return;
#endif
		void * pBytePtr = GetBytePtr();
		m_pStack[m_stackSize] = pBytePtr;
		m_stackSize++;
	}

	inline void Pop()
	{
		Assert(m_stackSize != 0);
		void * pBytePtr = m_pStack[m_stackSize - 1];
		SetBytePtr(pBytePtr);
		PopAllocTillBytePtr(pBytePtr);
		m_stackSize--;
	}

private:
	void * * m_pStack;
	uint32_t m_stackSize;
#ifdef DEBUG
	uint32_t m_maxStackSize;
#endif
};

template <uint32_t N, uint32_t POW2>
class MemAllocatorFixed : public IMemAllocator, public IMemAllocationReporter
{
public:
	MemAllocatorFixed()
	{
		m_pBytes = reinterpret_cast<uint8_t *>(_mm_malloc(N * (1 << POW2), 16));

		m_pAllocInfos = new MemAllocInfo[N];

		for (uint32_t i = 0; i < N; ++i)
		{
			MemAllocInfo * pAllocInfo = m_pAllocInfos + i;

			m_freeList.push_tail(pAllocInfo);
		}
	}

	virtual ~MemAllocatorFixed()
	{
		delete[] m_pAllocInfos;
		m_pAllocInfos = 0;

		_mm_free(m_pBytes);
		m_pBytes = 0;
	}

	virtual void * Alloc(size_t size, uint32_t tag = 0)
	{
		Assert(size == (1 << POW2));

		MemAllocInfo * pAllocInfo = m_freeList.pop_tail_value();

#ifdef _DEBUG
		pAllocInfo->Init(size, tag, 0);

		m_usedList.push_tail(pAllocInfo);
#endif

		uint32_t index = pAllocInfo - m_pAllocInfos;

		return m_pBytes + (index << POW2);
	}

	virtual void Free(void * p)
	{
		uint32_t index = PointerToIndex(p);

		MemAllocInfo * pAllocInfo = m_pAllocInfos + index;

	#ifdef _DEBUG
		m_usedList.erase(pAllocInfo);
	#endif

		m_freeList.push_tail(pAllocInfo);
	}

	virtual IMemAllocationIterator * GetAllocationIterator()
	{
		m_iterator.Begin(m_usedList);

		return &m_iterator;
	}

private:
	inline uint32_t PointerToIndex(void * p)
	{
		uintptr_t offset =
			reinterpret_cast<uintptr_t>(p) -
			reinterpret_cast<uintptr_t>(m_pBytes);

		Assert((offset & ((1 << POW2) - 1)) == 0);

		return static_cast<uint32_t>(offset >> POW2);
	}

	uint8_t * m_pBytes;
	MemAllocInfo * m_pAllocInfos;
	MemAllocInfoList m_usedList;
	MemAllocInfoList m_freeList;
	MemAllocInfoListIterator m_iterator;
};

inline void * operator new(size_t size, IMemAllocator * pAllocator)
{
	return pAllocator->Alloc(size);
}

inline void * operator new[](size_t size, IMemAllocator * pAllocator)
{
	return pAllocator->Alloc(size);
}

inline void operator delete(void * p, IMemAllocator * pAllocator)
{
	return pAllocator->Free(p);
}

inline void operator delete[](void * p, IMemAllocator * pAllocator)
{
	return pAllocator->Free(p);
}

//

extern MemAllocatorGeneric g_alloc;
