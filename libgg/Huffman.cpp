#include <algorithm>
#include "Debugging.h"
#include "Huffman.h"

struct FrequencyInfo
{
	size_t frequency;
	int symbol;

	inline bool operator<(const FrequencyInfo & other) const
	{
		return frequency > other.frequency;
	}
};

typedef FrequencyInfo FrequencyTable[256];

static void CalculateFrequencyTable(const uint8_t * bytes, size_t numBytes, FrequencyTable & frequencyTable)
{
	for (int i = 0; i < 256; ++i)
	{
		frequencyTable[i].symbol = i;
		frequencyTable[i].frequency = 0;
	}

	for (size_t i = 0; i < numBytes; ++i)
		frequencyTable[bytes[i]].frequency++;
}

void HuffTree::Create(const void * bytes, size_t numBytes)
{
	// calculate frequency table
	FrequencyTable frequencyTable;
	CalculateFrequencyTable(static_cast<const uint8_t*>(bytes), numBytes, frequencyTable);

	// sort symbols by frequency
	std::sort(frequencyTable, frequencyTable + 256);

	// setup decode and encode roots
	m_decodeRoot = new HuffNode;
	m_encodeRoots = new HuffNode*[256];
	memset(m_encodeRoots, 0, sizeof(HuffNode*) * 256);

	// create decode tree
	HuffNode * rootNode = m_decodeRoot;
	for (int i = 0; i < 256; ++i)
	{
		if (frequencyTable[i].frequency == 0)
			continue;

		// create node for the symbol being inserted

		HuffNode * node = new HuffNode;
		
		node->m_symbol = frequencyTable[i].symbol;
		node->m_weight = frequencyTable[i].frequency;

		m_encodeRoots[node->m_symbol] = node;

		// insert the symbol by traversing the tree, making sure the tree stays balanced

		HuffNode * parent = rootNode;

		for (;;)
		{
			if (parent->m_child[0] == 0)
			{
				if (parent == rootNode)
				{
					// root node is the only non-leaf node where child[0] is NULL

					parent->m_child[0] = node;
					node->m_parent = parent;
					break;
				}
				else
				{
					// child[0] is NULL, which must mean this is a leaf node, storing a symbol. create a new node
					// which will be parent for both the existing leaf node and the new node we're currently inserting

					HuffNode * oldParent = parent;

					parent = new HuffNode;
					parent->m_weight = oldParent->m_weight;
					parent->m_parent = oldParent->m_parent;
					parent->m_child[0] = oldParent;
					parent->m_child[1] = node;

					oldParent->m_parent = parent;
					node->m_parent = parent;

					if (oldParent == parent->m_parent->m_child[0])
						parent->m_parent->m_child[0] = parent;
					if (oldParent == parent->m_parent->m_child[1])
						parent->m_parent->m_child[1] = parent;
					break;
				}
			}
			else if (parent->m_child[1] == 0)
			{
				parent->m_child[1] = node;
				node->m_parent = parent;
				break;
			}
			else
			{
				// traversing down the path with the least weight

				if (parent->m_child[0]->m_weight < parent->m_child[1]->m_weight)
					parent = parent->m_child[0];
				else
					parent = parent->m_child[1];
			}
		}

		// add the weight of the current node to all of the ancestor tree nodes. the weight stored
		// at any node in the tree will always store the total weight down the entire tree branch
		for (HuffNode * temp = parent; temp != 0; temp = temp->m_parent)
			temp->m_weight += node->m_weight;
	}

#if 0
	CalculateFrequencyTable(static_cast<const uint8_t*>(bytes), numBytes, frequencyTable);
	for (int i = 0; i < 256; ++i)
	{
		if (frequencyTable[i].frequency == 0)
			continue;
		int numBits = 0;
		bool bits[256];
		for (HuffNode * node = m_encodeRoots[i]; node != 0; node = node->m_parent)
		{
			if (node->m_parent)
			{
				HuffNode * parent = node->m_parent;

				Assert(node == parent->m_child[0] || node == parent->m_child[1]);
				if (node == parent->m_child[0])
					bits[numBits++] = false;
				else
					bits[numBits++] = true;
			}
			else
				Assert(node == m_decodeRoot);
		}
		HuffNode * decodeNode = m_decodeRoot;
		for (int j = 0; j < numBits; ++j)
			decodeNode = decodeNode->m_child[bits[numBits - 1 - j] ? 1 : 0];
		Assert(decodeNode->m_symbol == i);
		printf("symbol %d: decoded=%d, frequency=%u, numBits=%d\n",
			i,
			(int)decodeNode->m_symbol,
			frequencyTable[i].frequency,
			numBits);
	}
#endif
}
