#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include "Parse.h"

int Parse::Int32(const std::string& text)
{
	return atoi(text.c_str());
}

unsigned int Parse::UInt32(const std::string& text)
{
	return (unsigned int)atol(text.c_str());
}

float Parse::Float(const std::string& text)
{
	return (float)atof(text.c_str());
}

bool Parse::Bool(const std::string& text)
{
	std::string temp = text;
	
	for (size_t i = 0; i < temp.length(); ++i)
		temp[i] = tolower(temp[i]);

	return temp == "true" || temp == "yes" || temp == "1";
}
