#include <algorithm>
#include <assert.h>
#include <string.h>
#include <xmmintrin.h>

#define FIXEDSHIFT 0

template <typename K, typename V, unsigned int S, unsigned int BITS>
class RadixSorter
{
public:
	static const unsigned int N = 1 << BITS;
	
	class Elem
	{
	public:
		K key;
		V value;
	};
	
	Elem data[2][S];

	Elem * elem;
	
	RadixSorter()
	{
		elem = data[0];
	}
	
	static inline unsigned int BucketIndex(unsigned int k, unsigned int shift)
	{
#if FIXEDSHIFT
		return k & ((1 << BITS) - 1);
#else
		return (k >> shift) & ((1 << BITS) - 1);
#endif
	}
	
	void Sort(unsigned int s, unsigned int bits)
	{
		assert(s <= S);
		
		unsigned int loopCount = (bits + BITS - 1) / BITS;
		unsigned int shift = 0;
	
		for (unsigned int loop = 0; loop < loopCount; ++loop)
		{
			Elem * __restrict src = data[(loop + 0) & 1];
			Elem * __restrict dst = data[(loop + 1) & 1];
			
#if FIXEDSHIFT
			// if this isn't the first loop, shift the sort key
			
			if (loop != 0)
			{
				for (unsigned int i = 0; i < s; ++i)
				{
					src[i].key >>= BITS;
				}
			}
#endif
			
			// calculate key counts for our bucket allocation
			
			unsigned int count[N];
			
			memset(count, 0, sizeof(count));
			
			for (unsigned int i = 0; i < s; ++i)
			{
				unsigned int k = BucketIndex(src[i].key, shift);
				
				count[k]++;
			}
			
			// calculate bucket allocation offsets
			
			unsigned int offset[N];
			
			unsigned int curOffset = 0;
			
			for (unsigned int i = 0; i < N; ++i)
			{
				offset[i] = curOffset;
				
				curOffset += count[i];
			}
			
			// insert
			
			unsigned int i = s;
			
			while (i >= 4)
			{
				const unsigned int k0 = BucketIndex(src[0].key, shift);
				const unsigned int k1 = BucketIndex(src[1].key, shift);
				const unsigned int k2 = BucketIndex(src[2].key, shift);
				const unsigned int k3 = BucketIndex(src[3].key, shift);
				
				const unsigned int elemOffset0 = offset[k0]++;
				const unsigned int elemOffset1 = offset[k1]++;
				const unsigned int elemOffset2 = offset[k2]++;
				const unsigned int elemOffset3 = offset[k3]++;
				
				dst[elemOffset0] = src[0];
				dst[elemOffset1] = src[1];
				dst[elemOffset2] = src[2];
				dst[elemOffset3] = src[3];
				
				src += 4;
				i -= 4;	
			}
			
			while (i >= 1)
			{
				const unsigned int k = BucketIndex(src[0].key, shift);
				
				const unsigned int elemOffset = offset[k]++;
				
				dst[elemOffset] = src[0];
				
				src += 1;
				i -= 1;
			}
			
			shift += BITS;
		}
		
		elem = data[loopCount & 1];
	}
};

static const unsigned int N = 1 << 16;

void testRadixSort()
{
	typedef RadixSorter<unsigned short, unsigned short, N, 8> Sorter;
	//typedef RadixSorter<unsigned int, unsigned int, N, 11> Sorter;
	//typedef RadixSorter<unsigned long, unsigned long, N, 11> Sorter;
	
	Sorter * sorter = new Sorter();
	
	for (unsigned int i = 0; i < N; ++i)
	{
		sorter->elem[i].key = rand();
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
		assert(sorter->elem[i].key <= sorter->elem[i + 1].key);
	}
#endif
	
	delete sorter;
	sorter = 0;
}
