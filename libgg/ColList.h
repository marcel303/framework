#pragma once

#include "MemAllocators.h"

namespace Col
{
	// List node. Used by list class.
	
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

	// List class.
	
	template <class T, class A = Mem::BasicAllocator< ListNode<T> > >
	class List
	{
	public:
		List()
		{
			m_Head = 0;
			m_Tail = 0;
			m_Count = 0;
		}

		~List()
		{
			while (m_Head)
				Remove(m_Head);
		}

		void LinkHead(ListNode<T>* node)
		{
			if (m_Count == 0)
			{
				m_Head = node;
				m_Tail = node;
			}
			else
			{
				m_Head->m_Prev = node;
				node->m_Next = m_Head;
				m_Head = node;
			}
			
			m_Count++;
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

		ListNode<T>* AddHead(const T& obj)
		{
			ListNode<T>* node = m_Allocator.Alloc();

			node->m_Object = obj;

			LinkHead(node);

			return node;
		}
		
		ListNode<T>* AddTail(const T& obj)
		{
			ListNode<T>* node = m_Allocator.Alloc();

			node->m_Object = obj;

			LinkTail(node);

			return node;
		}

		ListNode<T>* AddAfter(ListNode<T>* prev, const T& obj)
		{
			if (prev == m_Tail)
			{
				return AddTail(obj);
			}
			else
			{
				ListNode<T>* node = m_Allocator.Alloc();
				
				node->m_Object = obj;
				
				ListNode<T>* next = prev->m_Next;
				
				node->m_Prev = prev;
				node->m_Next = next;
				
				prev->m_Next = node;
				next->m_Prev = node;
				
				m_Count++;
				
				return node;
			}
		}
		
		void Remove(ListNode<T>* node)
		{
			UnLink(node);

			m_Allocator.Free(node);
		}

		void Clear()
		{
			while (m_Tail)
				Remove(m_Tail);
		}

		inline int Count_get() const
		{
			return m_Count;
		}

		ListNode<T>* FindNode(const T& obj)
		{
			for (ListNode<T>* node = m_Head; node; node = node->m_Next)
				if (node->m_Object == obj)
					return node;
			
			return 0;
		}

		A m_Allocator;
		ListNode<T>* m_Head;
		ListNode<T>* m_Tail;
		int m_Count;
	};
}
