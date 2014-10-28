#pragma once

#include "Debugging.h"
#include "Rect.h"

class RectNode
{
public:
	inline RectNode()
	{
		prev = 0;
		next = 0;
	}

#ifdef DEBUG
	~RectNode()
	{
		Assert(prev == 0);
		Assert(next == 0);
	}
#endif

	Rect rect;

	RectNode* prev;
	RectNode* next;
};

class RectList
{
public:
	RectList()
	{
		m_head = 0;
		m_tail = 0;
		m_count = 0;
	}

	~RectList()
	{
		Clear();
	}

	RectNode* m_head;
	RectNode* m_tail;
	int m_count;

	void Clear()
	{
		while (m_head)
			delete UnLink(m_head);

		Assert(m_head == 0);
		Assert(m_tail == 0);
		Assert(m_count == 0);
	}

	RectNode* UnLink(RectNode* node)
	{
		Assert(node);

		if (node == m_head)
			m_head = node->next;
		if (node == m_tail)
			m_tail = node->prev;

		if (node->prev)
			node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;

		node->prev = 0;
		node->next = 0;

		m_count--;

		return node;
	}

	RectNode* Acquire()
	{
		RectNode* result = 0;

		if (m_tail != 0)
		{
			result = m_tail;

			UnLink(m_tail);
		}
		else
		{
			result = new RectNode;
		}

		Assert(result->prev == 0);
		Assert(result->next == 0);

		return result;
	}

	void Release(RectNode* node)
	{
		Assert(node);

		// TODO: Move linkage to Link().

		if (m_tail)
			m_tail->next = node;

		node->prev = m_tail;
		node->next = 0;

		m_tail = node;

		if (m_head == 0)
			m_head = m_tail;

		m_count++;

		Assert(m_head && m_tail);
	}
};

class RectSet
{
public:
	RectList m_rects;

	void Add(const Rect& rect);	// doesn't insert, but copies rect
	//void Remove(RectListItr i);
	void Remove(RectNode* node);
	void Clear();

	bool AddClip(const Rect& rect);
	bool Sub(const Rect& subRect);
};
