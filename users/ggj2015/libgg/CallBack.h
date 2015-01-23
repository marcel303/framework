#pragma once

#include "Debugging.h"

typedef void (*CallBackCB)(void* obj, void* arg);

class CallBack
{
public:
	inline CallBack()
	{
		m_Obj = 0;
		m_CallBackCB = 0;
	}

	inline CallBack(void* obj, CallBackCB cb)
	{
		m_Obj = obj;
		m_CallBackCB = cb;
	}

	inline void Invoke(void* arg) const
	{
		m_CallBackCB(m_Obj, arg);
	}

	inline bool IsSet() const
	{
		return m_CallBackCB != 0;
	}
	
	inline void* Object_get() const
	{
		return m_Obj;
	}
	
	inline CallBackCB CallBack_get() const
	{
		return m_CallBackCB;
	}

private:
	void* m_Obj;
	CallBackCB m_CallBackCB;
};

template <int SIZE>
class CallBackList
{
public:
	CallBackList()
	{
		mCallBackListSize = 0;
	}

	~CallBackList()
	{
		mCallBackListSize = 0;
	}

	void Add(CallBack callBack)
	{
		Assert(mCallBackListSize != SIZE);
		Assert(callBack.IsSet());

		mCallBackList[mCallBackListSize++] = callBack;
	}

	bool IsSet() const
	{
		return mCallBackListSize != 0;
	}

	inline void Invoke(void* arg) const
	{
		Assert(IsSet());

		for (int i = 0; i < mCallBackListSize; ++i)
			mCallBackList[i].Invoke(arg);
	}

private:
	CallBack mCallBackList[SIZE];
	int mCallBackListSize;
};
