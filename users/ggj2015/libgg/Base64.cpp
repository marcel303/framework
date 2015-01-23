#include <stdlib.h>
#include "Base64.h"
#include "Debugging.h"
#include "StringEx.h"

static char encodingTable[64] =
{
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/' 
};

std::string Base64::Encode(const ByteString& data)
{
	std::string result;
	
	if (data.Length_get() == 0)
		return result;

	const uint8_t* bytes = &data.m_Bytes[0];
	const int byteCount = data.Length_get();
	
	int baseIndex = 0;
	
	unsigned char inbuf[3];
	unsigned char outbuf[4];
	
	for (;;)
	{
		const int todo = byteCount - baseIndex;
		
		if (todo <= 0)
			break;
		
		for (int i = 0; i < 3; i++)
		{
			int index = baseIndex + i;
			
			if (index < byteCount)
				inbuf[i] = bytes[index];
			else
				inbuf[i] = 0;
		}
		
		outbuf[0] = (inbuf [0] & 0xFC) >> 2;
		outbuf[1] = ((inbuf [0] & 0x03) << 4) | ((inbuf [1] & 0xF0) >> 4);
		outbuf[2] = ((inbuf [1] & 0x0F) << 2) | ((inbuf [2] & 0xC0) >> 6);
		outbuf[3] = inbuf [2] & 0x3F;
		
		int copyCount = 4;
		
		switch (todo)
		{
			case 1: 
				copyCount = 2; 
				break;
			case 2: 
				copyCount = 3; 
				break;
		}
		
		for (int i = 0; i < copyCount; i++)
			result.push_back(encodingTable[outbuf[i]]);
		
		for (int i = copyCount; i < 4; i++)
			result.push_back('=');
		
		baseIndex += 3;
	}
	
	return result;
}

ByteString Base64::Decode(const std::string& text)
{
	ByteString result;
	
	const char *characters = &text[0];
	const int characterCount = (int)text.length();
	
	int characterIndex = 0;
	
	unsigned char inbuf[4];
	unsigned char outbuf[4];
	
	int inbufIndex = 0;
	
	for (;;)
	{
		if (characterIndex >= characterCount)
			break;
		
		unsigned char character = characters[characterIndex++];
		
		bool ignore = false;
		bool end = false;
		
		if ((character >= 'A' ) && (character <= 'Z')) character = character - 'A';
		else if((character >= 'a' ) && (character <= 'z')) character = character - 'a' + 26;
		else if((character >= '0' ) && (character <= '9')) character = character - '0' + 52;
		else if(character == '+' ) character = 62;
		else if(character == '=' ) end = true;
		else if(character == '/' ) character = 63;
		else ignore = true; 
		
		if (!ignore)
		{
			int outbufSize = 3;
			bool stop = false;
			
			if (end)
			{
				if (!inbufIndex)
					break;
				
				if ((inbufIndex == 1) || (inbufIndex == 2))
					outbufSize = 1;
				else
					outbufSize = 2;
				
				inbufIndex = 3;
				stop = true;
			}
			
			inbuf[inbufIndex++] = character;
			
			if (inbufIndex == 4)
			{
				inbufIndex = 0;
				
				outbuf[0] = (inbuf[0] << 2) | ((inbuf[1] & 0x30) >> 4);
				outbuf[1] = ((inbuf[1] & 0x0F) << 4) | ((inbuf[2] & 0x3C) >> 2);
				outbuf[2] = ((inbuf[2] & 0x03) << 6) | (inbuf[3] & 0x3F);
				
				for (int i = 0; i < outbufSize; i++) 
					result.m_Bytes.push_back(outbuf[i]);
			}
			
			if (stop)
				break;
		}
	}
	
	return result;
}

std::string Base64::HttpFix(const std::string& text)
{
	std::string temp;
	
	temp = String::Replace(text, '+', '-');
	temp = String::Replace(temp, '/', '_');
	
	return temp;
}

std::string Base64::HttpUnFix(const std::string& text)
{
	std::string temp;
	
	temp = String::Replace(text, '-', '+');
	temp = String::Replace(temp, '_', '/');
	
	return temp;
}

static void TestBase64(const std::string& text)
{
	std::string text1 = text;
	ByteString bytes1 = ByteString::FromString(text1);
	std::string text2 = Base64::Encode(bytes1);
	ByteString bytes2 = Base64::Decode(text2);
	std::string text3 = bytes2.ToString();
	std::string text4 = Base64::HttpFix(text3);
	std::string text5 = Base64::HttpUnFix(text4);
	
	Assert(text1 == text3);
	Assert(text3 == text5);
}

void Base64::DBG_SelfTest()
{
	TestBase64("Hello World");
	
	for (int i = 0; i < 1000; ++i)
	{
		// generste random string
		
		std::string text;
		
		for (int j = 0; j < 100; ++j)
		{
			char c = rand() % 256;
			
			text.push_back(c);
		}
		
		TestBase64(text);
	}
}
