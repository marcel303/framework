#pragma once

#include <string>
#include <vector>

class Path
{
public:
	Path();
	Path(const std::string& path);
	
	void Parse(const std::string& a_Path);
	void MakeRelative(const Path& a_BaseDirectory, const Path& a_Path);
	void MakeCompact();
	std::string ToString() const;
	
	static std::string GetFileName(const std::string& path);
	static std::string StripExtension(const std::string& path); // Removes the final extension
	static std::string ReplaceExtension(const std::string& path, const std::string& extension);
	static std::string GetBaseName(const std::string& path); // Removes any extensions added to the file name and removes any directories as well
	static std::string GetExtension(const std::string& path);
	static std::string GetDirectory(const std::string& path);
	static std::string MakeAbsolute(const std::string& base, const std::string& path);
	static std::string MakeRelative(const std::string& base, const std::string& path1, const std::string& path2);
	
private:
	static std::string NormalizeSlashes(const std::string& path);
	static std::string Sanitize(const std::string& path);
	static std::vector<std::string> CombinePathComponents(const std::vector<std::string>& components1, const std::vector<std::string>& components2);
	static std::string FlattenPathComponents(const std::vector<std::string>& components);
	static std::vector<std::string> GetPathComponents(const std::string& path);
	
	static char SafeRead(const std::string& a_String, int a_Position);
	
	std::vector<std::string> m_Nodes;
};
