#ifndef FILE_H
#define FILE_H
#pragma once

#include <stdio.h>
#include <string>
#include "SharedPtr.h"

enum FILE_OPENMODE
{
	FILE_READ,
	FILE_READ_TEXT,
	FILE_WRITE,
	FILE_WRITE_TEXT
};

enum FILE_SEEKMODE
{
	SEEKMODE_ABS,
	SEEKMODE_REL
};

class File
{
public:
	File();
	~File();

	bool Open(const std::string& filename, FILE_OPENMODE mode);
	bool Close();

	bool Opened();

	std::string Contents();

	bool Read(void* dst, int size);
	bool Write(const void* src, int size);
	bool Write(const std::string& v);

	bool Seek(int position, FILE_SEEKMODE mode);

	int GetLength();
	int GetPosition();

	FILE* m_file;
	std::string m_filename;
};

typedef SharedPtr<File> ShFile;

#endif
