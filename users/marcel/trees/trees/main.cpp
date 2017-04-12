#include "../../../../libgg/Debugging.h"
#include "../../../../libgg/RadixSorter.h"
#include "../../../../libgg/RedBlackTree.h"
#include <algorithm>
#include <assert.h>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//

static const unsigned int kMaxDataSetSize = 1 << 16;

//

static void testRadixSort(unsigned short * __restrict dataSet, const unsigned int dataSetSize)
{
	typedef RadixSorter<unsigned short, unsigned short, kMaxDataSetSize, 8> Sorter;
	
	Sorter * sorter = new Sorter();
	
	clock_t time = 0;
	
	time -= clock();
	
	for (unsigned int i = 0; i < dataSetSize; ++i)
	{
		sorter->elem[i].key = dataSet[i];
	}
	
	sorter->Sort(dataSetSize, 16);
	
#if 1
	for (unsigned int i = 0; i < dataSetSize; ++i)
	{
		dataSet[i] = sorter->elem[i].key;
	}
#endif

	time += clock();
	
	printf("testRadixSort: time: %gms\n", float(time) / CLOCKS_PER_SEC * 1000.f);
	
	delete sorter;
	sorter = 0;
}

//

static void testRedBlackTree(unsigned short * __restrict dataSet, const unsigned int dataSetSize)
{
	typedef RedBlackTree<unsigned short> Tree;
	
	Tree t;
	
	Tree::Node * nodes = new Tree::Node[dataSetSize];
	
	memset(nodes, 0, sizeof(Tree::Node) * dataSetSize);
	
	clock_t time = 0;
	
	time -= clock();
	
	for (unsigned int i = 0; i < dataSetSize; ++i)
	{
		nodes[i].SetValue(dataSet[i]);
	}
	
	for (unsigned int i = 0; i < dataSetSize; ++i)
	{
		Tree::Node & n = nodes[i];
		
		t.insert(&n);
	}
	
#if 1
	unsigned int c = 0;
	
	for (Tree::iterator i = t.begin(); i != t.end(); ++i)
	{
		const unsigned short v = i.get_value();
		
		dataSet[c] = v;
		
		++c;
	}
#endif

	time += clock();
	
#if 1
	printf("testRedBlackTree: time = %gms\n", float(time) / CLOCKS_PER_SEC * 1000.f);
#endif
	
	delete[] nodes;
	nodes = 0;
}

//

static void testStdSort(unsigned short * dataSet, const unsigned int dataSetSize)
{
	clock_t time = 0;
	
	time -= clock();
	
	std::sort(dataSet, dataSet + dataSetSize);
	
	time += clock();
	
	printf("testStdSort: time = %gms\n", float(time) / CLOCKS_PER_SEC * 1000.f);
}

//

static void verifySortingResult(const unsigned short * dataSet, const unsigned int dataSetSize)
{
	for (unsigned int i = 0; i < dataSetSize - 1; ++i)
	{
		Assert(dataSet[i] <= dataSet[i + 1]);
	}
}

//

int main (int argc, const char * argv[])
{
	const unsigned int N = kMaxDataSetSize;
	
	unsigned short originalDataSet[N];
	
	for (unsigned int i = 0; i < N; ++i)
	{
		originalDataSet[i] = rand() + rand() * RAND_MAX;
	}
	
	unsigned short dataSet[N];
	
	//
	
	memcpy(dataSet, originalDataSet, sizeof(dataSet));
	testRadixSort(dataSet, N);
	verifySortingResult(dataSet, N);
	
	memcpy(dataSet, originalDataSet, sizeof(dataSet));
	testRedBlackTree(dataSet, N);
	verifySortingResult(dataSet, N);
	
	memcpy(dataSet, originalDataSet, sizeof(dataSet));
	testStdSort(dataSet, N);
	verifySortingResult(dataSet, N);
	
	//
	
	return 0;
}
