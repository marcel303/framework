#include <stdio.h>
#include "ParseString.h"
#include "ParseTokenizer.h"

namespace Parse
{
	const static Token skipTokenArray[] = // List of whitespace characters.
	{
		Token( 0, TK_NEWLINE, "\n" ),
		Token( 0, TK_NONE,    "\r" ),
		Token( 0, TK_NONE,    "\t" ),
		Token( 0, TK_NONE,    " "  ),
		Token( 0, TK_ZERO,    ""   )
	};

	Tokenizer::Tokenizer()
	{
		tokenZero.token = TK_ZERO;
		
 		optionCheckLiteralCollisions = 1;
		optionVerboseErrors = 0;
		
		errorFlag = 0;
	}

	Tokenizer::~Tokenizer()
	{
		Clear();
	}

	int Tokenizer::GetTokenCount() const
	{
		return (int)tokenArray.size();
	}

	const Token* Tokenizer::GetToken(int index) const
	{
		if (index < 0 || index >= (int)tokenArray.size())
		{
			//const_cast<Tokenizer*>(this)->SetError("Error: Token index out of range.\n");
			//return 0;
			return &tokenZero;
		}
		
		return tokenArray[index];
	}
		
	void Tokenizer::Clear()
	{
		// Clear token array.
		
		for (unsigned int i = 0; i < tokenArray.size(); ++i)
			delete tokenArray[i];
		
		tokenArray.clear();
	}

	int Tokenizer::Tokenize(String& text, const Token* tokenArray)
	{
		Clear();
		
		line = 0;
		lineStart = text.SeekGetPosition();
		
		int lastToken = TK_ZERO;
		int lastLiteral = 0;
		
		while (!text.SeekIsEnd())
		{
			Token* currentToken = 0;
			
			if (tokenArray)
			{
				for (int i = 0; !currentToken && tokenArray[i].token != TK_ZERO; ++i)
				{
					if (Match(text, &tokenArray[i]))
					{
						currentToken = (Token*)&tokenArray[i];
					
						text.Seek(currentToken->string.GetLength(), String::SK_ADD);
					}
				}
			}
			
			for (int i = 0; !currentToken && skipTokenArray[i].token != TK_ZERO; ++i)
			{
				if (Match(text, &skipTokenArray[i]))
				{
					currentToken = (Token*)&skipTokenArray[i];
				
					text.Seek(currentToken->string.GetLength(), String::SK_ADD);
				}
			}		
			
			Token tempToken;
			
			if (!currentToken)
			{
				tempToken.token = TK_ZERO;
				tempToken.literal = 1;

				int charactersRead;
				
				if (
					ParseString(text, &tempToken, &charactersRead) ||
					ParseUnenclosedString(text, &tempToken, &charactersRead) ||
					ParseHexString(text, &tempToken, &charactersRead) ||
					ParseBinaryString(text, &tempToken, &charactersRead) ||
					ParseNumber(text, &tempToken, &charactersRead))
				{
				}
				
				if (errorFlag)
					return 0;
				
				if (tempToken.token != TK_ZERO)
				{
					currentToken = &tempToken;
					text.Seek(charactersRead, String::SK_ADD);
				}
			}

			if (!currentToken)
			{
				char temp[512];
				sprintf(temp, "Error: Tokeniser: Invalid input detected: Line: %d Column: %d", line + 1, text.SeekGetPosition() - lineStart);
				SetError(temp);
				return 0;
			}
			
			if (currentToken->token == TK_NONE)
			{
			}
			else if (currentToken->token == TK_NEWLINE)
			{
				// TODO: Newline as token?
				line++;
				lineStart = text.SeekGetPosition();
			}
			else
			{
				// Any other token we add to our list.
				
				// Prevent two literals next to each other, seperated by.. nothing. More comfortable to add this here
				// instead of in all the other places.
				
				if (optionCheckLiteralCollisions && (lastLiteral && currentToken->literal))
				{
					char temp[512];
					
					sprintf(temp, "Error: Tokeniser: Literal collision detected: Line: %d Column: %d (", line + 1, text.SeekGetPosition() - lineStart);
					
					uint32_t length = (uint32_t)strlen(temp);
					for (uint32_t i = 0; i < currentToken->string.GetLength(); ++i)
					{
						temp[length + i] = currentToken->string.GetChar(i);
						temp[length + i + 1] = 0;
					}
			
					char temp2[] = ")\n";
					strcat(temp, temp2);
					
					SetError(temp);
					
					return 0;
				}

				Token* token = new Token;
				
				*token = *currentToken;

				AddToken(token);		
		
				lastToken = currentToken->token;
				lastLiteral = currentToken->literal;		
			}
		}
		
		return 1;
	}

	int Tokenizer::Match(const String& text, const Token* match) const
	{
		// Ensure literal is enclosed by non-literal characters.
		if (match->literal)
			if (text.IsLiteral(-1) || text.IsLiteral(match->string.GetLength()))
				return 0;
		
		// Compare strings character by character.
		for (uint32_t i = 0; i < match->string.GetLength(); ++i)
			if (text.GetChar(i) != match->string.GetChar(i))
				return 0;

		return 1;
	}

	void Tokenizer::AddToken(Token* token)
	{
		tokenArray.push_back(token);
	}

	int Tokenizer::ParseString(const String& string, Token* token, int* charactersRead) const
	{
		// Strings must begin and end with a '"'.
		if (string.GetChar(0) != '\"')
			return 0;
		
		Token temp;
		temp.token = TK_STRING;
		
		int i = 1;
		
		for (; string.GetChar(i) != '\"' && string.GetChar(i); ++i)
			temp.string.Concatenate(string.GetChar(i));
		
		if (string.GetChar(i) != '\"')
			return 0;
		
		temp.literal = 1;
		
		token->token = temp.token;
		token->string = temp.string;
		token->literal = temp.literal;
		
		*charactersRead = token->string.GetLength() + 2;
		
		return 1;
	}

	int Tokenizer::ParseUnenclosedString(const String& string, Token* token, int* charactersRead) const
	{
		// Enenclosed strings must begin with a character.
		if (!string.IsCharacter(0))
			return 0;

		token->token = TK_STRING;
		
		// String end with first non-literal character.
		for (int i = 0; string.IsLiteral(i); ++i)
				token->string.Concatenate(string.GetChar(i));

		token->literal = 1;					
		
		*charactersRead = token->string.GetLength();
		
		return 1;
	}

	int Tokenizer::ParseHexString(const String& string, Token* token, int* charactersRead) const
	{
		if (string.GetChar(0) != '0' || string.GetChar(1) != 'x' || !string.IsHexNumber(2))
			return 0;
		
 		token->token = TK_INTEGER;

		token->string.Concatenate(string.GetChar(0));
		token->string.Concatenate(string.GetChar(1));
		
		int value = 0;
		
		for (int i = 2; string.IsHexNumber(i); ++i)
		{
			int temp = string.GetHexNumber(i);
			
			value = value * 16 + temp;
			
			token->string.Concatenate(string.GetChar(i));
		}
		
		token->vI = value;
		token->literal = 1;				

		*charactersRead = token->string.GetLength();
		
  		return 1;
	}

	int Tokenizer::ParseBinaryString(const String& string, Token* token, int* charactersRead) const
	{
		if (string.GetChar(0) != '0' || string.GetChar(1) != 'b' || !string.IsBitNumber(2))
			return 0;
		
 		token->token = TK_INTEGER;

		token->string.Concatenate(string.GetChar(0));
		token->string.Concatenate(string.GetChar(1));
		
		int value = 0;
		
		for (int i = 2; string.IsBitNumber(i); ++i)
		{
			int temp = string.GetBinaryNumber(i);
			
			value = value * 2 + temp;
			
			token->string.Concatenate(string.GetChar(i));
		}
		
		token->vI = value;
		token->literal = 1;				
		
		*charactersRead = token->string.GetLength();
		
  		return 1;
	}

	int Tokenizer::ParseNumber(const String& string, Token* token, int* charactersRead) const
	{
		if (string.GetChar(0) != '-' && !string.IsNumber(0))
			return 0;
		
		// Must be either float or integer.
		int dotCount = 0;
		
		int sign = 1;
		
		int integer = 0;
		
		float real = 0.0f;
		float realScale = 0.1f;
		
		String temp;
		
		int i;
		
		for (i = 0; string.GetChar(i) == '-' || string.IsNumber(i) || string.GetChar(i) == '.'; ++i)
		{
			if (string.GetChar(i) == '-')
			{
				if (i == 0)
					sign = -1;
				else
				{
				
					char temp[512];
					sprintf(temp, "Error: Tokeniser: Invalid number notation: Line: %d Column: %d", line + 1, string.SeekGetPosition() - lineStart);
					const_cast<Tokenizer*>(this)->SetError(temp);

					return 0;						
				}
			}
			else if (string.GetChar(i) == '.')
			{
				if (dotCount != 0)
				{
					// Float with multiple dots.. yaah rite..
					char temp[512];
					sprintf(temp, "Error: Tokeniser: Invalid float notation: Line: %d Column: %d", line + 1, string.SeekGetPosition() - lineStart);
					const_cast<Tokenizer*>(this)->SetError(temp);
					
					return 0;						
				}
				
				dotCount++;
			}
			else
			{
				if (dotCount == 0)
				{
					integer = integer * 10 + string.GetNumber(i);
				}
				else
				{
					real += string.GetNumber(i) * realScale;
					realScale *= 0.1f;
				}
			}
			
			temp.Concatenate(string.GetChar(i));
		}
		
		if (string.IsLiteral(i))
		{
			char temp[512];
			sprintf(temp, "Error: Tokeniser: Garbage after number: Line: %d Column: %d", line + 1, string.SeekGetPosition() - lineStart);
			const_cast<Tokenizer*>(this)->SetError(temp);
			
			return 0;
		}
		
		// Float?
		if (dotCount == 1)
		{
			token->token = TK_FLOAT;				
			token->vF = (integer + real) * sign;
		}
		else
		{
			token->token = TK_INTEGER;								
			token->vI = integer * sign;
		}
		
		token->string = temp;
		token->literal = 0;

		*charactersRead = token->string.GetLength();
		
		return 1;		
	}

	void Tokenizer::SetOption(int option, int value)
	{
		if (option == OPTION_CHECK_LITERAL_COLLISIONS)
		{
			optionCheckLiteralCollisions = value;
		}
		else if (option == OPTION_VERBOSE_ERRORS)
		{
			optionVerboseErrors = value;
		}
		else
		{
			SetError("Error: Invalid option.\n");
		}
	}

	int Tokenizer::GetOption(int option) const
	{
		if (option == OPTION_CHECK_LITERAL_COLLISIONS)
		{
			return optionCheckLiteralCollisions;
		}
		else if (option == OPTION_VERBOSE_ERRORS)
		{
			return optionVerboseErrors;
		}
		
		const_cast<Tokenizer*>(this)->SetError("Error: Invalid option.\n");
		
		return 0;
	}

	void Tokenizer::SetError(char* errorMessage)
	{
		if (errorMessage)
		{
			strcpy(this->errorMessage, errorMessage);
		}
		else
		{
			strcpy(this->errorMessage, "Error: Unspecified error.\n");
		}
			
		errorFlag = 1;
		
		if (optionVerboseErrors)
		{
			printf(this->errorMessage);
		}
	}

	const char* Tokenizer::GetErrorMessage() const
	{
		if (errorFlag)
		{
			const_cast<Tokenizer*>(this)->errorFlag = 0;
			return errorMessage;
		}
			
		return 0;
	}
}
