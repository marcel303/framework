#pragma once

#include <stdio.h>
#include <string>

class FileReader
{
public:
	FileReader()
	{
		file = 0;
	}
	
	~FileReader()
	{
		close();
	}
	
	bool open(const char * filename, bool textMode)
	{
		file = fopen(filename, textMode ? "rt" : "rb");
		
		return file != 0;
	}
	
	void close()
	{
		if (file != 0)
		{
			fclose(file);
			file = 0;
		}
	}
	
	template <typename T>
	bool read(T & dst)
	{
		return fread(&dst, sizeof(dst), 1, file) == 1;
	}
	
	bool read(void * dst, int numBytes)
	{
		return fread(dst, numBytes, 1, file) == 1;
	}
	
	bool read(std::string & dst)
	{
		char line[1024];
		if (fgets(line, sizeof(line), file) == 0)
			return false;
		else
		{
			dst = line;
			return true;
		}
	}
	
	bool skip(int numBytes)
	{
		return fseek(file, numBytes, SEEK_CUR) == 0;
	}
	
	FILE * file;
};
