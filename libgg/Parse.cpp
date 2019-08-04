#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include "Parse.h"

static bool strEqual(const char * a, const char * b)
{
	size_t i = 0;
	
	for (;;)
	{
		if (a[i] != b[i])
			return false;
		
		if (a[i] == 0)
			return true;
		
		++i;
	}
	
	return false;
}

int Parse::Int32(const char * text)
{
	return atoi(text);
}

unsigned int Parse::UInt32(const char * text)
{
	return (unsigned int)atol(text);
}

float Parse::Float(const char * text)
{
	return (float)atof(text);
}

bool Parse::Bool(const char * text)
{
	const size_t maxSize = 4;
	
	char temp[maxSize + 1];
	
	size_t text_size = 0;
	
	while (text[text_size] != 0)
	{
		if (text_size == maxSize)
			return false;
		
		temp[text_size] = tolower(text[text_size]);
		
		text_size++;
	}

	temp[text_size] = 0;
	
	return
		strEqual(temp, "true") ||
		strEqual(temp, "yes") ||
		strEqual(temp, "1");
}
