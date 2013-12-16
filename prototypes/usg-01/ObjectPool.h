#pragma once

#if 0
// todo: implement pooling

template <class T>
class ObjectPool : ObjectAllocator<T>
{
public:
	virtual T* Alloc()
	{
		return new T();
	}

	virtual void Free(T* obj)
	{
		delete obj;
	}

	//List<T> m_Pool;
};

#endif
