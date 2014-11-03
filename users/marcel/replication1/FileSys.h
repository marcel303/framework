#ifndef FILESYS_H
#define FILESYS_H
#pragma once

#include <string>
#include "File.h"

class FileSys
{
public:
	FileSys(std::string root);
	virtual ~FileSys();

	virtual bool FileExists(std::string filename) = 0;
	virtual bool FileOpen(std::string filename, FILE_OPENMODE mode, ShFile& out_file) = 0;

	std::string GetRoot();

private:
	std::string m_root;
};

#endif
