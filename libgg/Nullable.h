#pragma once

#include "Exception.h"

template <class T>
class Nullable
{
public:
	Nullable()
	{
		mHasValue = false;
	}
	
	Nullable(T value)
	{
		mValue = value;
		mHasValue = true;
	}
	
	void Reset()
	{
		mHasValue = false;
	}
	
	bool HasValue_get()
	{
		return mHasValue;
	}
	
	T Value_get() const
	{
		if (!mHasValue)
			throw ExceptionVA("value not set");
		
		return mValue;
	}
	
	void Value_set(T value)
	{
		mValue = value;
		mHasValue = true;
	}
	
	Nullable<T>& operator=(T value)
	{
		Value_set(value);
		
		return *this;
	}
	
private:
	bool mHasValue;
	T mValue;
};
