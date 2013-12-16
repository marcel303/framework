#include "FileStream.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StringEx.h"
#include "Subversion.h"

bool SVN::GetRevision(const std::string& directory, int& out_Revision)
{
	try
	{
		std::string fileName = directory + ".svn\\entries";
		
		std::string line = GetLine(fileName, 4);
		
		out_Revision = Parse::Int32(line);
		
		return true;
	}
	catch (...)// (std::exception& e)
	{
		out_Revision = 0;
		
		return false;
	}
}

std::string SVN::GetLine(const std::string& fileName, int line)
{
	FileStream stream(fileName.c_str(), OpenMode_Read);
	StreamReader reader(&stream, false);
	
	std::vector<std::string> lines = reader.ReadAllLines();
	
	if (line < 0 || line >= (int)lines.size())
		return String::Empty;
		
	return lines[line];
}
