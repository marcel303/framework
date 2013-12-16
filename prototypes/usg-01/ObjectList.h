#pragma once

//#include "Allocators.h"

template <class T>
class ListNode
{
public:
	ListNode()
	{
		m_Prev = 0;
		m_Next = 0;
		m_Object = T();
	}

	void UnLink()
	{
		if (m_Prev)
			m_Prev->m_Next = m_Next;
		if (m_Next)
			m_Next->m_Prev = m_Prev;

		m_Next = 0;
		m_Prev = 0;
	}

	ListNode<T>* m_Prev;
	ListNode<T>* m_Next;
	T m_Object;
};

template <class T, class A>
class List
{
public:
	List()
	{
		m_Head = 0;
		m_Tail = 0;
		m_Count = 0;
	}

	void LinkTail(ListNode<T>* node)
	{
		if (m_Count == 0)
		{
			m_Head = node;
			m_Tail = node;
		}
		else
		{
			m_Tail->m_Next = node;
			node->m_Prev = m_Tail;
			m_Tail = node;
		}

		m_Count++;
	}

	void UnLink(ListNode<T>* node)
	{
		if (node == m_Head)
			m_Head = node->m_Next;
		if (node == m_Tail)
			m_Tail = node->m_Prev;

		node->UnLink();

		m_Count--;
	}

	ListNode<T>* AddTail(T obj)
	{
		ListNode<T>* node = m_Allocator.Alloc();

		node->m_Object = obj;

		LinkTail(node);

		return node;
	}

	void Remove(ListNode<T>* node)
	{
		UnLink(node);

		m_Allocator.Free(node);
	}

	int Count_get() const
	{
		return m_Count;
	}

	A m_Allocator;
	ListNode<T>* m_Head;
	ListNode<T>* m_Tail;
	int m_Count;
};
