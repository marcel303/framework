#pragma once

#include <assert.h>

template <typename T, int C>
struct MemoryPool
{
	T pool[C];
};

template <typename T, int C>
struct FastList
{
	struct Node
	{
		Node * next = nullptr;

		T value;
	};

	Node * storage = nullptr;
	int nextNodeIndex = 0;

	Node * head = nullptr;
	Node * tail = nullptr;

	FastList()
	{
		storage = (Node*)malloc(C * sizeof(Node));
	}
	
	~FastList()
	{
		free(storage);
	}
	
	Node * allocNode()
	{
		assert(nextNodeIndex < C);
		
		return storage + nextNodeIndex++;
	}

	void push_back(const T & value)
	{
		Node * node = allocNode();

		node->value = value;
		node->next = nullptr;

		if (tail != nullptr)
			tail->next = node;
		else
			head = node;
		
		tail = node;
	}

	void insert_after(Node * node, const T & value)
	{
		Node * new_node = allocNode();

		new_node->value = value;
		new_node->next = node->next;
		
		node->next = new_node;
		
		if (node == tail)
			tail = new_node;
	}

	void reset()
	{
		nextNodeIndex = 0;

		head = nullptr;
		tail = nullptr;
	}
};
