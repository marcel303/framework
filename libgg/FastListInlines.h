#pragma once

#include "FastList.h"

template <typename T>
inline FastList<T>::FastList()
	: m_pHead(0)
	, m_pTail(0)
	, m_size(0)
{
}

template <typename T>
inline FastList<T>::~FastList()
{
}

template <typename T>
inline void FastList<T>::push_head(T * pElem)
{
	assert(pElem->m_pPrev == 0 && pElem->m_pNext == 0);

	if (m_pHead == 0)
	{
		m_pHead = m_pTail = pElem;
	}
	else
	{
		m_pHead->m_pPrev = pElem;
		pElem->m_pNext = m_pHead;
		m_pHead = pElem;
	}

	m_size++;
}

template <typename T>
inline void FastList<T>::push_tail(T * pElem)
{
	assert(pElem->m_pPrev == 0 && pElem->m_pNext == 0);

	if (m_pHead == 0)
	{
		m_pHead = m_pTail = pElem;
	}
	else
	{
		m_pTail->m_pNext = pElem;
		pElem->m_pPrev = m_pTail;
		m_pTail = pElem;
	}

	m_size++;
}

template <typename T>
inline void FastList<T>::pop_tail()
{
	erase(m_pTail);
}

template <typename T>
inline T * FastList<T>::pop_tail_value()
{
	T * pElem = m_pTail;

	erase(pElem);

	return pElem;
}

template <typename T>
inline void FastList<T>::erase(T * pElem)
{
	T * pPrev = pElem->m_pPrev;
	T * pNext = pElem->m_pNext;

	if (pElem == m_pHead)
		m_pHead = pNext;
	else
	{
		pPrev->m_pNext = pNext;
		pElem->m_pPrev = 0;
	}

	if (pElem == m_pTail)
		m_pTail = pPrev;
	else
	{
		pNext->m_pPrev = pPrev;
		pElem->m_pNext = 0;
	}

	m_size--;
}

template <typename T>
inline void FastList<T>::discard()
{
	m_pHead = m_pTail = 0;
	m_size = 0;
}

template <typename T>
inline T * FastList<T>::head() const
{
	return m_pHead;
}

template <typename T>
inline T * FastList<T>::tail() const
{
	return m_pTail;
}

template <typename T>
inline uint32_t FastList<T>::size() const
{
	return m_size;
}

// FastList::fwd_iterator

template <typename T>
inline FastList<T>::fwd_iterator::fwd_iterator(T * pElem)
	: m_pElem(pElem)
{
}

template <typename T>
inline bool FastList<T>::fwd_iterator::has_next() const
{
	return m_pElem && m_pElem->m_pNext;
}

template <typename T>
inline void FastList<T>::fwd_iterator::next()
{
	m_pElem = m_pElem->m_pNext;
}

template <typename T>
inline void FastList<T>::fwd_iterator::operator++()
{
	next();
}

template <typename T>
inline T * FastList<T>::fwd_iterator::get()
{
	return m_pElem;
}

template <typename T>
inline const T * FastList<T>::fwd_iterator::get() const
{
	return m_pElem;
}
