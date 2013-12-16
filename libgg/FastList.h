#pragma once

#include <stdint.h>

/* A lightweight doubly linked list.
 * Elements stored in the list act as list nodes as well.
 * The requirement is these elements have a pointer to the previous
 * and next element called m_pPrev and m_pNext which must be set
 * to 0 initially.
 */
 
template <typename T>
class FastList
{
public:
	inline FastList();
	inline ~FastList();

	// adding / removing
	
	inline void push_head(T * pElem);
	inline void push_tail(T * pElem);
	inline void pop_tail();
	inline T * pop_tail_value();
	inline void erase(T * pElem);
	inline void discard();

	// access

	inline T * head() const;
	inline T * tail() const;
	inline uint32_t size() const;
	
	// iteration
	
	class fwd_iterator
	{
	public:
		inline fwd_iterator(T * pElem);
		inline bool has_next() const;
		inline void next();
		inline void operator++();
		inline T * get();
		inline const T * get() const;

	private:
		T * m_pElem;
	};

	typedef fwd_iterator iterator;
	typedef fwd_iterator const_iterator;

	inline iterator begin() { return iterator(m_pHead); }
	inline const_iterator begin() const { return const_iterator(m_pHead); }

private:
	T * m_pHead;
	T * m_pTail;
	uint32_t m_size;
};

#include "FastListInlines.h"
