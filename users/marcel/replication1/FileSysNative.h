#ifndef FILESYSNATIVE_H
#define FILESYSNATIVE_H
#pragma once

#include <map>
#include "FileSys.h"

// TODO: To scan or not to scan?

class FileSysNative : public FileSys
{
public:
	FileSysNative(std::string root);
	~FileSysNative();

	virtual bool FileExists(std::string filename);
	virtual bool FileOpen(std::string filename, FILE_OPENMODE mode, ShFile& out_file);

private:
	std::string Complete(std::string filename);
};

#endif
