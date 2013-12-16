#pragma once

typedef int xHandle;

class HandlePool
{
public:
	HandlePool();
	~HandlePool();

	void Setup(int count);

	xHandle Allocate();
	void Free(xHandle handle);

private:
	xHandle* mHandleList;
	int mHandleListLength;
	int mFreeHandleCount;
};
