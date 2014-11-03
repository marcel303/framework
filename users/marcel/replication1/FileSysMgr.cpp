#include <algorithm>
#include "Debug.h"
#include "FileSysMgr.h"

FileSysMgr::FileSysMgr()
{
}

FileSysMgr::~FileSysMgr()
{
	while (m_filesystems.size() > 0)
	{
		delete m_filesystems.back().m_fs;
		m_filesystems.pop_back();
	}
}

void FileSysMgr::Add(FileSys* fs, int priority)
{
	FASSERT(fs);

	FileSysItem item;

	item.m_fs = fs;
	item.m_priority = priority;

	m_filesystems.push_back(item);

	std::sort(m_filesystems.begin(), m_filesystems.end());
}

bool FileSysMgr::OpenFile(std::string filename, FILE_OPENMODE mode, ShFile& out_file)
{
	for (size_t i = 0; i < m_filesystems.size(); ++i)
	{
		if (m_filesystems[i].m_fs->FileExists(filename))
			return m_filesystems[i].m_fs->FileOpen(filename, mode, out_file);
	}

	return false;
}
