#pragma once

#include <string.h>

template <int SIZE>
class FixedSizeString
{
public:
	FixedSizeString()
	{
		mSize = 0;
	}

	FixedSizeString(const char* c)
	{
		mSize = 0;

		append(c);
	}

	inline void clear()
	{
		mSize = 0;
	}

	inline bool empty() const
	{
		return mSize == 0;
	}

	const char* c_str() const { const_cast<FixedSizeString<SIZE>*>(this)->mText[mSize] = 0; return mText; }

	inline void append(const char* c)
	{
		while (*c && mSize < SIZE)
		{
			mText[mSize++] = *c;
			c++;
		}
	}

	inline void push_back(char c)
	{
		if (mSize < SIZE)
			mText[mSize++] = c;
	}

	inline int find(char c)
	{
		for (int i = 0; i < mSize; ++i)
			if (mText[i] == c)
				return i;
		return -1;
	}

	inline FixedSizeString<SIZE> substr(int offset, int length) const
	{
		FixedSizeString<SIZE> result;

		for (int i = 0; i < length; ++i)
			result.push_back(mText[offset + i]);

		return result;
	}

	inline void operator=(const char* c)
	{
		mSize = 0;
		append(c);
	}

	inline void operator+=(const char* c)
	{
		append(c);
	}

	inline void Replace(char src, char dst)
	{
		for (int i = 0; i < mSize; ++i)
			if (mText[i] == src)
				mText[i] = dst;
	}

	inline bool operator<(const FixedSizeString& other) const
	{
		int size = mSize < other.mSize ? mSize : other.mSize;

		for (int i = 0; i < size; ++i)
		{
			if (mText[i] < other.mText[i])
				return true;
			if (mText[i] > other.mText[i])
				return false;
		}

		return mSize < other.mSize;
	}

	inline bool operator==(const FixedSizeString& other) const
	{
		return strcmp(c_str(), other.c_str()) == 0;
	}

private:
	char mText[SIZE + 1];
	int mSize;
};
