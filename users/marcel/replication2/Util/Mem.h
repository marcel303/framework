#ifndef MEM_H
#define MEM_H
#pragma once

template <typename T>
inline void SAFE_FREE(T*& ptr)
{
	if (ptr)
	{
		delete ptr;
		ptr = 0;
	}
}

//#define SAFE_FREE(x) if (x) { delete x; x = 0; }
#define SAFE_FREE_ARR(x) if (x) { delete[] x; x = 0; }

#define COPY_PROTECT(T) \
	T(const T& other) \
	{ \
		Assert(0); \
	} \
	void operator=(const T& other) \
	{ \
		Assert(0); \
	}

#define foreach(item, collection) for (collection::iterator item = collection.begin, item != collection.end(); ++item)

#endif
