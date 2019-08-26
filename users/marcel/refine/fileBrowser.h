#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#ifndef _MSC_VER
	#include <dirent.h>
#endif

#ifdef _MSC_VER
	#include <direct.h>
#endif

struct FileElem
{
	bool isFolded = true;

	std::string name;
	std::string path;
	bool isFile = false;

	void fold()
	{
		isFolded = true;
	}

	void unfold()
	{
		isFolded = false;
	}
};

struct FileBrowser
{
	std::string rootPath;
	
	std::map<std::string, std::vector<FileElem>*> elemsByPath;
	
	std::function<void (const std::string & filename)> onFileSelected;

	void init(const char * in_rootPath);

	void clearFiles();
	void scanFiles(const char * path);

	void tickRecurse(const char * path);
	void tick();
};
