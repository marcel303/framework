#pragma once

#include <string>

class PathCtx
{
public:
	PathCtx(const std::string& directory);
	~PathCtx();
	
	void MakeCurrent();
	std::string MakeAbsolute(const std::string& path) const;
	std::string MakeRelative(const std::string& path) const;
	
private:
	std::string m_Directory;
};
