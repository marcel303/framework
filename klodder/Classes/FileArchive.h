#pragma once

#include <string>
#include <vector>
#include "libgg_forward.h"
#include "MemoryStream.h"

class FileArchiveMember
{
public:
	std::string mFileName;
	MemoryStream mData;
};

class FileArchive
{
public:
	~FileArchive();
	
	void Clear();
	void Add(const char* name, Stream* stream, int length);
	void Add(const char* name, Stream* stream);
	void Load(Stream* stream);
//	void Save(Stream* stream);
	void SaveBegin(Stream* stream);
	void SaveAdd(Stream* stream, const char* name, Stream* dataStream, int length);
	void SaveAdd(Stream* stream, const char* name, Stream* dataStream);
	void SaveEnd(Stream* stream);
	
	Stream* GetStream(std::string name);
	
private:
	std::vector<FileArchiveMember*> mFileList;
};
