#pragma once

#include <stdint.h>

class HuffNode
{
public:
	HuffNode()
	{
		memset(this, 0, sizeof(HuffNode));
	}

	size_t m_weight;
	int m_symbol;

	HuffNode * m_parent;
	HuffNode * m_child[2];
};

class HuffTree
{
	void FreeRecursively(HuffNode * node)
	{
		for (int i = 0; i < 2; ++i)
			if (node->m_child[i])
				FreeRecursively(node->m_child[i]);
		delete node;
	}
public:
	HuffTree()
		: m_decodeRoot(0)
		, m_encodeRoots(0)
	{
	}

	~HuffTree()
	{
		FreeRecursively(m_decodeRoot);
		m_decodeRoot = 0;

		delete [] m_encodeRoots;
		m_encodeRoots = 0;
	}

	void Create(const void * bytes, size_t numBytes);

	HuffNode * m_decodeRoot;
	HuffNode ** m_encodeRoots;
};
