#pragma once

#include <stdio.h>
#include <string>

class FileReader
{
public:
	FileReader()
	{
		file = 0;
		ownsFile = false;
	}
	
	FileReader(FILE * in_file)
	{
		file = in_file;
		ownsFile = false;
	}
	
	~FileReader()
	{
		if (ownsFile)
			close();
	}
	
	bool open(const char * filename, bool textMode)
	{
		file = fopen(filename, textMode ? "rt" : "rb");
		ownsFile = true;
		
		return file != 0;
	}
	
	void close()
	{
		if (file != 0)
		{
			fclose(file);
			file = 0;
			ownsFile = false;
		}
	}
	
	template <typename T>
	bool read(T & dst)
	{
		return fread(&dst, sizeof(dst), 1, file) == 1;
	}
	
	bool read(void * dst, size_t numBytes)
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
	
	bool skip(size_t numBytes)
	{
		return fseek(file, numBytes, SEEK_CUR) == 0;
	}
	
	size_t position() const
	{
		return ftell(file);
	}
	
	bool seek(size_t position)
	{
		return fseek(file, position, SEEK_SET) == 0;
	}
	
	FILE * file;
	bool ownsFile;
};
