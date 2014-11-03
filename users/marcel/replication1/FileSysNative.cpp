#include "FileSysNative.h"

FileSysNative::FileSysNative(std::string root) : FileSys(root)
{
}

FileSysNative::~FileSysNative()
{
}

bool FileSysNative::FileExists(std::string filename)
{
	File file;

	return file.Open(Complete(filename), FILE_READ);
}

bool FileSysNative::FileOpen(std::string filename, FILE_OPENMODE mode, ShFile& out_file)
{
	bool result = true;

	out_file = ShFile(new File);

	if (!out_file->Open(Complete(filename), mode))
		result = false;

	return result;
}

std::string FileSysNative::Complete(std::string filename)
{
	return GetRoot() + filename;
}
