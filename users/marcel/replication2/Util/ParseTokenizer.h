#ifndef PARSETOKENIZER_H
#define PARSETOKENIZER_H
#pragma once

#include <vector>
#include "ParseString.h"
#include "Types.h"

// NOTE: Code taken from e-merge project.

namespace Parse
{
	enum TokenEnum
	{
		TK_ZERO = 0, ///< 'Zero' token.
		TK_NONE,
		TK_NEWLINE, ///< Newline.
	//	TK_HEX,
	//	TK_BINARY,
		TK_INTEGER, ///< Integer number. Value stored in vI.
		TK_FLOAT, ///< Floating point number. Value stored in vF.
		TK_STRING, ///< String.
		TK_CUSTOM = 100 ///< Beginning of user defined tokens.
	};

	class Token
	{
	public:
		Token()
		{
			literal = 0;
			token = TK_ZERO;
			vI = 0;
			vF = 0.0f;
		}
		Token(int literal, int token, const char* string)
		{
			this->literal = literal;
			this->token = token;
			this->string = String(string);
			vI = 0;
			vF = 0.0f;
		}

		int literal; ///< If set to 1, this is a literal token (consisting of regular characters) and thus the string must be isolated by non literal token-string to be tokenised. Else, the entire literal is considered.
		int token; ///< Token.
		String string; ///< String that belongs to token.
		
		int vI; ///< Value of integer number token.
		float vF; ///< Value of float number token.
	};

	class Tokenizer
	{
	public:
		Tokenizer();
		~Tokenizer();

 		void SetOption(int option, int value); ///< Set tokenizer options.
 		int GetOption(int option) const;       ///< Get tokenizer option.
	 	
		int GetTokenCount() const;              ///< Get number of generated tokens.
		const Token* GetToken(int index) const; ///< Get token at specified index.
		void Clear();                           ///< Clear array of generated tokens.

		int Tokenize(String& text, const Token* tokenArray); ///< Tokenize input text using specified user defined tokens.

 		const char* GetErrorMessage() const;                 ///< Get error message.

		const Token* operator[](int index) const
		{
			return GetToken(index);
		}

 		const static int OPTION_CHECK_LITERAL_COLLISIONS = 0; ///< Check for literal collisions.
		const static int OPTION_VERBOSE_ERRORS = 1;           ///< Show verbose errors. Log to stdout.

	protected:
		int Match(const String& text, const Token* match) const;  ///< Return 1 if text matches token. 0 otherwise.
		void AddToken(Token* token);                              ///< Add token to array of generated tokens.

		int ParseString(const String& string, Token* token, int* charactersRead) const;           ///< Parse string. Return 1 on success. 0 otherwise.
		int ParseUnenclosedString(const String& string, Token* token, int* charactersRead) const; ///< Parse string. Return 1 on success. 0 otherwise.
		int ParseHexString(const String& string, Token* token, int* charactersRead) const;        ///< Parse hex string. Return 1 on success. 0 otherwise.
		int ParseBinaryString(const String& string, Token* token, int* charactersRead) const;     ///< Parse binary string. Return 1 on success. 0 otherwise.
		int ParseNumber(const String& string, Token* token, int* charactersRead) const;           ///< Parse number. Return 1 on success. 0 otherwise.

 		void SetError(const char* errorMessage); ///< Set error message.

		Token tokenZero;                ///< Zero token, returned if token index out of bounds.
		std::vector<Token*> tokenArray; ///< Array of generated tokens.
		
		int line;
		int lineStart;

 		int optionCheckLiteralCollisions;
 		int optionVerboseErrors;
	 	
 		char errorMessage[512];
 		int errorFlag; 	
	};
}

#endif
