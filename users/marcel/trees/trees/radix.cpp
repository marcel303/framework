#include "../../../../libgg/Debugging.h"
#include "../../../../libgg/RadixSorter.h"
#include <algorithm>
#include <stdio.h>

#ifdef WIN32
	#include <time.h>
#endif

static const unsigned int N = 1 << 16;

void testRadixSort()
{
	typedef RadixSorter<unsigned short, unsigned short, N, 8> Sorter;
	//typedef RadixSorter<unsigned int, unsigned int, N, 11> Sorter;
	//typedef RadixSorter<unsigned long, unsigned long, N, 11> Sorter;
	
	Sorter * sorter = new Sorter();
	
	for (unsigned int i = 0; i < N; ++i)
	{
		sorter->elem[i].key = rand() + rand() * RAND_MAX;
		//sorter->elem[i].key = rand() % N;
		//sorter->elem[i].key = rand() % (N / 4);
		//sorter->elem[i].key = i;
	}
	
	for (unsigned int r = 0; r < 1; ++r)
	{
		clock_t time = 0;
		
		time -= clock();
		
//		sorter->Sort(N, 32);
		sorter->Sort(N, 16);
		
		time += clock();
		
		printf("time: %gms\n", float(time) / CLOCKS_PER_SEC * 1000.f);
	}
	
#if !FIXEDSHIFT
	for (unsigned int i = 0; i < N - 1; ++i)
	{
		Assert(sorter->elem[i].key <= sorter->elem[i + 1].key);
	}
#endif
	
	delete sorter;
	sorter = 0;
}
