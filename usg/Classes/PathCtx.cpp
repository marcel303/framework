#include "PathCtx.h"

#if 0

PathCtx::PathCtx(const std::string& directory)
{
	m_Directory = directory;
}

PathCtx::~PathCtx()
{
}

void PathCtx::MakeCurrent()
{
	cwd(m_Directory.c_str());
}

std::string PathCtx::MakeAbsolute(const std::string& path) const
{
	// todo
	
	return String::Empty;
}

std::string PathCtx::MakeRelative(const std::string& path) const
{
	Path path1;
	Path path2;

	path1.Parse(m_Directory);
	path2.Parse(path);

	path1.MakeCompact();
	path2.MakeCompact();

	Path path3;

	path3.MakeRelative(path1, path2);
	path3.MakeCompact();

	return path3.ToString();
}

#endif
