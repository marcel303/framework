#ifndef FILESYSMGR_H
#define FILESYSMGR_H
#pragma once

#include <vector>
#include "FileSys.h"

class FileSysMgr
{
public:
	static FileSysMgr& I()
	{
		static FileSysMgr mgr;
		return mgr;
	}

	void Add(FileSys* fs, int priority);
	bool OpenFile(std::string filename, FILE_OPENMODE mode, ShFile& out_file);

private:
	class FileSysItem
	{
	public:
		inline bool operator<(FileSysItem item)
		{
			return m_priority < item.m_priority;
		}

		FileSys* m_fs;
		int m_priority;
	};

	FileSysMgr();
	~FileSysMgr();

	std::vector<FileSysItem> m_filesystems;
};

#endif
