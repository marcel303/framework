#pragma once

#include "Debugging.h"

// note: std::priority_queue outperforms BinaryHeap with MSVC, so maybe use it instead of this class if you want top performance

template <typename T, int CAPACITY>
struct BinaryHeap
{
	T nodes[CAPACITY];
	int size;

	BinaryHeap()
		: size(0)
	{
	}

	void push(const T & t)
	{
		Assert(size < CAPACITY);

		int index = size;

		size++;

		nodes[index] = t;

		for (;;)
		{
			if (index == 0)
				break;
			
			const int parent = (index - 1) >> 1;

			if (nodes[parent] < nodes[index])
			{
				const T temp = nodes[index];
				nodes[index] = nodes[parent];
				nodes[parent] = temp;

				index = parent;
			}
			else
			{
				break;
			}
		}
	}

	void pop()
	{
		Assert(size > 0);

		int index = 0;

		nodes[index] = nodes[size - 1];

		size--;

		for (;;)
		{
			const int c1 = (index << 1) + 1;
			const int c2 = (index << 1) + 2;

			int bestIndex = index;

			if (c1 < size && nodes[bestIndex] < nodes[c1])
				bestIndex = c1;
			if (c2 < size && nodes[bestIndex] < nodes[c2])
				bestIndex = c2;

			if (bestIndex != index)
			{
				const T temp = nodes[index];
				nodes[index] = nodes[bestIndex];
				nodes[bestIndex] = temp;

				index = bestIndex;
			}
			else
			{
				break;
			}
		}
	}

	bool empty() const
	{
		return size == 0;
	}

	const T & top() const
	{
		return nodes[0];
	}
};
