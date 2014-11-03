#include "FileSys.h"

FileSys::FileSys(std::string root)
{
	m_root = root;
}

FileSys::~FileSys()
{
}

std::string FileSys::GetRoot()
{
	return m_root;
}
