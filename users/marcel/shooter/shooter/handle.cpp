#include <exception>
#include "handle.h"

HandlePool::HandlePool()
{
	mHandleList = 0;
	mHandleListLength = 0;
	mFreeHandleCount = 0;
}

HandlePool::~HandlePool()
{
	Setup(0);
}

void HandlePool::Setup(int count)
{
	delete[] mHandleList;
	mHandleList = 0;
	mHandleListLength = 0;
	mFreeHandleCount = 0;

	if (count > 0)
	{
		mHandleList = new xHandle[count];
		mHandleListLength = count;
		mFreeHandleCount = mHandleListLength;
		for (int i = 0; i < count; ++i)
			mHandleList[i] = i;
	}
}

xHandle HandlePool::Allocate()
{
	if (mFreeHandleCount == 0)
		throw std::exception();//("handle pool depleted");

	mFreeHandleCount--;

	return mHandleList[mFreeHandleCount];
}

void HandlePool::Free(xHandle handle)
{
	mHandleList[mFreeHandleCount] = handle;

	mFreeHandleCount++;
}
