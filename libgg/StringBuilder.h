#pragma once

#include <stdlib.h>
#include "Log.h" // VA_SPRINTF

template <int SIZE>
class FixedSizeStringBuilder
{
public:
	FixedSizeStringBuilder()
	{
		mLength = 0;
		mIsValid = true;
	}
	
	bool IsValid() const
	{
		return mIsValid;
	}

	void Append(char c)
	{
		if (mLength + 1 >= SIZE)
		{
			mIsValid = false;
			return;
		}

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
		
		if (v == 0)
		{
			// a common case is zero
			
			Append('0');
		}
		else
		{
			const bool isNegative = v < 0;
			
			long long vll = isNegative ? -static_cast<long long>(v) : v;
			
			int length = 0;
			
			while (vll != 0)
			{
				temp[length++] = '0' + (vll % 10);
				
				vll /= 10;
			}
			
			if (isNegative)
				temp[length++] = '-';
			
			if (mLength + length >= SIZE)
			{
				mIsValid = false;
				return;
			}
			
			while (length > 0)
			{
				--length;
				
				mBuffer[mLength] = temp[length];
				mLength++;
			}
		}
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
	bool mIsValid;
};

//

#include <string>

class StringBuilder
{
public:
	StringBuilder()
	{
		mText.reserve(1 << 16);
	}
	
	void Append(const char c)
	{
		mText.push_back(c);
	}
	
	void Append(const char * text)
	{
		mText.append(text);
	}
	
	void AppendFormat(const char * format, ...);
	
	const char* ToString()
	{
		return mText.c_str();
	}

private:

	std::string mText;

};
