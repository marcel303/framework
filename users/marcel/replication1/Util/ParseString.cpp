#include "ParseString.h"

namespace Parse
{
	void String::Construct()
	{
		base = 0;

		charBuffer = 0;
		charBufferSize = 0;
	}

	String::String()
	{
		Construct();
	}

	String::String(const String& string)
	{
		Construct();

		Concatenate(string);
	}

	String::String(const std::string& string)
	{
		Construct();

		Concatenate(string);
	}

	String::String(const char* string)
	{
		Construct();
	 	
 		Concatenate(string);
	}

	String::~String()
	{
		SetCharBufferSize(0);
	}

	int String::GetEffectiveIndex(int index) const
	{
		int effectiveIndex = index + base;
		
		// Check if "effectiveIndex" lies within the string's limits. If not, return -1.
		
		if (effectiveIndex < 0 || effectiveIndex > (int)GetLength() - 1)
			return -1;
			
		return effectiveIndex;
	}

	uint32_t String::GetLength() const
	{
		return (int)string.size();
	}

	char String::GetChar(int index) const
	{
		int effectiveIndex = GetEffectiveIndex(index);
		
		// Return 0 if the effective index lies outside the string.
		
		if (effectiveIndex < 0 || effectiveIndex >= (int)string.size())
			return 0;
		
		// Return the character at the effective index.
	 	
		return string[effectiveIndex];
	}

	void String::Clear()
	{
		string.clear();
	}

	void String::SeekStart()
	{
		base = 0;
	}

	void String::Seek(int offset, int mode)
	{
		// Assign?
		
		if (mode == SK_SET)
			base = offset;
			
		// Add?
		
		if (mode == SK_ADD)
			base += offset;
	}

	int String::SeekGetPosition() const
	{
		return base;
	}

	void String::SeekEnd()
	{
		// GetLength() - 1 is the last character of the string. GetLength() lies 1 character past the end.
		
		base = GetLength();
	}

	int String::SeekIsEnd() const
	{
		if (base >= (int)GetLength())
			return 1;
			
		return 0;
	}

	void String::Concatenate(const String& string)
	{
		// For every character in string, copy it to our string.
		
		for (unsigned int i = 0; i < string.string.size(); ++i)
			this->string.push_back(string.string[i]);
	}

	void String::Concatenate(const std::string& string)
	{
		// For every character in string, copy it to our string.
		
		for (unsigned int i = 0; i < string.size(); ++i)
			this->string.push_back(string[i]);
	}

	void String::Concatenate(const char* string)
	{
		// For every character in string, copy it to our string.
		
		for (int i = 0; string[i]; ++i)
			this->string.push_back(string[i]);
	}

	void String::Concatenate(char c)
	{
		// Copy single character to the string.
		
		string.push_back(c);
	}

	void String::ToLower()
	{
		for (uint32_t i = 0; i < string.size(); ++i)
		{
			char c = string[i];
			if (c >= 'A' && c <= 'Z')
				string[i] = c - 'A' + 'a';
		}
	}
		
	void String::SetCharBufferSize(int size)
	{
		// Do nothing if we already allocated a buffer of size "size".
		
		if (size == charBufferSize)
			return;
			
		if (charBuffer)
			delete[] charBuffer;
		
 		charBuffer = 0;
	  	
  		// If size < 0, do not allocate a new buffer.
	  	
		if (size <= 0)
			return;
		
		// Allocate a new buffer.
	 	
		charBufferSize = size;
		
		charBuffer = new char[size];
	}

	String::operator const char*() const
	{
		// Allocate buffer to hold our string copied to character array.
		
		const_cast<String*>(this)->SetCharBufferSize(GetLength() + 1);

		// Copy our string to the character array.
		
		for (unsigned int i = 0; i < string.size(); ++i)
			const_cast<String*>(this)->charBuffer[i] = string[i];
			
		// Terminate character array.
		
		const_cast<String*>(this)->charBuffer[GetLength()] = 0;
		
		return charBuffer;
	}

	String& String::operator=(const String& string)
	{
		Clear();
		
		Concatenate(string);
	 	
		return *this;
	}

	String& String::operator=(const char* string)
	{
		Clear();
		
		Concatenate(string);
		
		return *this;
	}

	int String::IsCharacter(int index) const
	{
		int c = GetChar(index);
		
		// Return 1 if the character is (a-z | A-Z) or '_'. Else 0.
	 	
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '_')		
			return 1;
			
		return 0;
	}

	int String::IsNumber(int index) const
	{
		int c = GetChar(index);
	 	
		// Return 1 if the character is (0-9). Else 0.
	  	
		if (c >= '0' && c <= '9')
 			return 1;
			
		return 0;
	}

	int String::IsHexNumber(int index) const
	{
		int c = GetChar(index);
		
		// Return 1 if the character is (0-9 | a-f | A-F). Else 0.
	   	
		if (c >= '0' && c <= '9')
 			return 1;
		if (c >= 'a' && c <= 'f')
 			return 1;
		if (c >= 'A' && c <= 'F')
 			return 1;    		
			
		return 0;
	}

	int String::IsBitNumber(int index) const
	{
		int c = GetChar(index);
		
		// Return 1 if the character is (0-1). Else 0.
	 	
		if (c >= '0' && c <= '1')
 			return 1;
			
		return 0;
	}

	int String::IsLiteral(int index) const
	{
		// Return 1 if the character at "base + index" is either a character or number.
		
		int c = GetChar(index);
		
		if (c == '.' || c == '_' || c == '\\' || c == '/')
			return 1;
		
		return IsCharacter(index) || IsNumber(index);
	}

	int String::IsWhitespace(int index) const
	{
		int c = GetChar(index);
		
		if (c == ' ' || c == '\t' || c == '\n' || c == 0)
			return 1;
			
		return 0;
	}

	int String::GetNumber(int index) const
	{
		return GetChar(index) - '0';
	}

	int String::GetHexNumber(int index) const
	{
		int c = GetChar(index);
		
		if (c >= '0' && c <= '9')
			c = c - '0';
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else
			c = c - 'A' + 10;

		return c;
	}

	int String::GetBinaryNumber(int index) const
	{
		return GetNumber(index);
	}
};
