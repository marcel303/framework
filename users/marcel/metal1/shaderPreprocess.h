#pragma once

#include <string>
#include <vector>

bool preprocessShader(
	const std::string & source,
	std::string & destination,
	const int flags,
	std::vector<std::string> & errorMessages,
	int & fileId);

bool preprocessShaderFromFile(
	const char * filename,
	std::string & destination,
	const int flags,
	std::vector<std::string> & errorMessages);
