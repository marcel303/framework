#pragma once

#include <stdlib.h>
#include <string.h>
#include "Log.h"

template <int SIZE>
class StringBuilder
{
public:
	StringBuilder()
	{
		mLength = 0;
	}

	void Append(char c)
	{
		if (mLength + 1 >= SIZE)
			return;

		mBuffer[mLength] = c;
		mLength++;
	}

	void Append(const char* text)
	{
		while (*text)
		{
			Append(*text);

			text++;
		}
	}

	void Append(int v)
	{
		char temp[32];
#if defined(WIN32) && 0
		_itoa(v, temp, 10);
#else
	// todo : make this faster
		sprintf(temp, "%d", v);
		//itoa(v, temp, 10);
#endif
		Append(temp);
	}

	void Append(float v)
	{
		char temp[32];
		//ftoa(v, temp);
		sprintf(temp, "%f", v);
		Append(temp);
	}

	void AppendFormat(const char* text, ...)
	{
		VA_SPRINTF(text, temp, text);

		Append(temp);
	}

	const char* ToString()
	{
		mBuffer[mLength] = 0;

		return mBuffer;
	}

private:

	char mBuffer[SIZE];
	int mLength;
};
