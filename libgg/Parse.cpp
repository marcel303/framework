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

// conversion with error checking

bool Parse::Int32(const char * text, int32_t & result)
{
	char * endptr;

	result = (int32_t)strtol(text, &endptr, 10);
	
	return endptr != text;
}

bool Parse::UInt32(const char * text, uint32_t & result)
{
	char * endptr;

	result = (uint32_t)strtoul(text, &endptr, 10);
	
	return endptr != text;
}

bool Parse::Float(const char * text, float & result)
{
	char * endptr;

	result = (float)strtod(text, &endptr);
	
	return endptr != text;
}

bool Parse::Double(const char * text, double & result)
{
	char * endptr;

	result = strtod(text, &endptr);
	
	return endptr != text;
}

bool Parse::Bool(const char * text, bool & result)
{
	result = Bool(text);
	
	return true;
}

// conversion without error checking

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

double Parse::Double(const char * text)
{
	return atof(text);
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
