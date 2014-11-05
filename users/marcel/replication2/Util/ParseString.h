#ifndef PARSESTRING_H
#define PARSESTRING_H
#pragma once

// NOTE: Code taken from e-merge project.

#include <string>
#include <vector>
#include "Types.h"

namespace Parse
{
	class String
	{
	public:
		String();
		String(const String& string);	
		String(const std::string& string);
		String(const char* string);
		~String();

		uint32_t GetLength() const;    ///< Return the length of the string.
		char GetChar(int index) const; ///< Get the character at location "index + base". If the effective index lies outside the string, 0 is returned.
		void Clear();                  ///< Clear the string.

		void SeekStart();                ///< Reset "base" to 0.
		void Seek(int offset, int mode); ///< Modify "base" by either assigning or adding "offset" to it.
		int  SeekGetPosition() const;    ///< Return "base".
		void SeekEnd();                  ///< Set "base" 1 character beyond the end of the string.
		int  SeekIsEnd() const;          ///< Return 1 if base is at the end of the string. Else 0.
		
		void Concatenate(const String& string);      ///< Add the characters of "string" to the end of the string.
		void Concatenate(const std::string& string); ///< Add the characters of "string" to the end of the string.
		void Concatenate(const char* string);        ///< Add the characters of "string" to the end of the string.
		void Concatenate(char c);                    ///< Add the character "c" to the string.
		
		void ToLower(); ///< Convert string to lower case.
		
		operator const char*() const;                 ///< Return a character array representing the string.
		String& operator=(const String& string);      ///< Copy "string" to the string.
		String& operator=(const std::string& string); ///< Copy "string" to the string.
		String& operator=(const char* string);        ///< Copy "string" to the string.
		
		int IsCharacter(int index) const;  ///< Return true if the character at "base + index" is an alpha character (a-z | A-Z) or '_'.
		int IsNumber(int index) const;     ///< Return true if the character at "base + index" is a numeric character (0-9).
		int IsHexNumber(int index) const;  ///< Return true if the character at "base + index" is a hexidecimal character (0-F).
		int IsBitNumber(int index) const;  ///< Return true if the character at "base + index" is a binary character (0-1).
		int IsLiteral(int index) const;    ///< Return true if the character at "base + index" is an alpha-numeric character (a-z | A-Z | 0-9) or any of the following: '.', '_', '\' or '/'.
		int IsWhitespace(int index) const; ///< Return true if the character at "base + index" is whitespace.
		
		int GetNumber(int index) const;       ///< Return the numeric conversion of the decimal character at "base + index".
		int GetHexNumber(int index) const;    ///< Return the numeric conversion of the hexidecimal character at "base + index".
		int GetBinaryNumber(int index) const; ///< Return the numeric conversion of the binary character at "base + index".

		const static int SK_SET = 0; ///< Assign "offset" to "base".
		const static int SK_ADD = 1; ///< Add "offset" to "base".

	protected:
		void SetCharBufferSize(int size);       ///< Set the size of the character buffer.
		int GetEffectiveIndex(int index) const;	///< Calculate "index + base". Returns -1 if the result'd be outside the string.

		char* charBuffer;   ///< Character buffer. Contains the actual string data.
		int charBufferSize; ///< Character buffer size.

	private:
		void Construct(); ///< Shared constructor.

		std::vector<char> string;
		int base;
	};
}

#endif
