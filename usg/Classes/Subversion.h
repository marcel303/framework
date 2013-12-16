#pragma once

#include <string>

class SVN
{
	static bool GetRevision(const std::string& directory, int& out_Revision);
	static std::string GetLine(const std::string& fileName, int line);
};
