#include "Path.h"
#include "StringEx.h"

Path::Path()
{
}

Path::Path(const std::string& path)
{
	Parse(path);
}

void Path::Parse(const std::string& path)
{
	std::string temp = Sanitize(path);

	m_Nodes = String::Split(temp, '/');
}

void Path::MakeRelative(const Path& a_BaseDirectory, const Path& a_Path)
{
	int n1 = (int)a_BaseDirectory.m_Nodes.size();
	int n2 = (int)a_Path.m_Nodes.size();

	int n = n1 < n2 ? n1 : n2;

	int i;

	for (i = 0; i < n && a_BaseDirectory.m_Nodes[i] == a_Path.m_Nodes[i]; ++i);

	for (int j = i; j < n1; ++j)
		m_Nodes.push_back("..");

	//m_Nodes = a_BaseDirectory.m_Nodes;

	for (; i < n2; ++i)
		m_Nodes.push_back(a_Path.m_Nodes[i]);
}

void Path::MakeCompact()
{
	std::vector<std::string> temp;

	for (size_t i = 0; i < m_Nodes.size(); ++i)
	{
		if (i != 0 && m_Nodes[i] == ".." && m_Nodes[i - 1] != "..")
		{
			temp.pop_back();
		}
		else
		{
			temp.push_back(m_Nodes[i]);
		}
	}

	m_Nodes = temp;
}

std::string Path::ToString() const
{
	std::string result;

	for (size_t i = 0; i < m_Nodes.size(); ++i)
	{
		if (i != 0)
			result += "/";

		result += m_Nodes[i];
	}

	return result;
}

//

std::string Path::GetFileName(const std::string& path)
{
	std::vector<std::string> components = Path::GetPathComponents(path);
	
	if (components.size() == 0)
		return String::Empty;
	
	return components[components.size() - 1];
}

std::string Path::StripExtension(const std::string& path)
{
	//path = GetFileName(path);
	
	size_t pos = path.find_last_of('.');
	
	if (pos != std::string::npos)
		return path.substr(0, pos);
	else
		return path;
}

std::string Path::ReplaceExtension(const std::string& path, const std::string& extension)
{
	return StripExtension(path) + "." + extension;
}

std::string Path::GetBaseName(const std::string& _path)
{
	std::string path = GetFileName(_path);
	
	size_t pos = path.find_first_of('.');
	
	if (pos != std::string::npos)
		return path.substr(0, pos);
	else
		return path;
}

std::string Path::GetExtension(const std::string& path, const bool toLower)
{
	size_t pos = path.find_last_of('.');
	
	if (pos == std::string::npos)
		return String::Empty;
	else
		pos++;
	
	if (toLower)
		return String::ToLower(path.substr(pos));
	else
		return path.substr(pos);
}

std::string Path::GetDirectory(const std::string& path)
{
	std::vector<std::string> components = Path::GetPathComponents(path);
	
	if (components.size() == 0)
		return String::Empty;
		
	components.pop_back();
	
	return FlattenPathComponents(components);
}

std::string Path::MakeAbsolute(const std::string& base, const std::string& path)
{
	std::vector<std::string> components1 = Path::GetPathComponents(base);
	std::vector<std::string> components2 = Path::GetPathComponents(path);
	
	std::vector<std::string> components = Path::CombinePathComponents(components1, components2);
	
	return Path::FlattenPathComponents(components);
}

std::string Path::MakeRelative(const std::string& base, const std::string& _path1, const std::string& _path2)
{
	std::string path1 = Path::MakeAbsolute(base, _path1);
	std::string path2 = Path::MakeAbsolute(base, _path2);
	
	std::vector<std::string> components1 = Path::GetPathComponents(path1);
	std::vector<std::string> components2 = Path::GetPathComponents(path2);
	
	size_t begin;
	
	for (begin = 0; begin < components1.size() && begin < components2.size() && components1[begin] == components2[begin]; begin++);
	
	std::vector<std::string> components;
	
	for (size_t i = begin; i < components2.size(); ++i)
		components.push_back(components2[i]);
	
	return Path::FlattenPathComponents(components);
}

std::string Path::NormalizeSlashes(const std::string& path)
{
	return String::Replace(path, '\\', '/');
}

std::string Path::Sanitize(const std::string& path)
{
	std::string result;

	for (size_t i = 0; i < path.size();)
	{
		char c1 = SafeRead(path, (int)i + 0);
		char c2 = SafeRead(path, (int)i + 1);

		if (c1 == '/' && c2 == '/')
		{
			result += c1;

			i += 2;
		}
		else
		{
			result += c1;

			i += 1;
		}
	}

	return result;
}

std::vector<std::string> Path::CombinePathComponents(const std::vector<std::string>& components1, const std::vector<std::string>& components2)
{
	std::vector<std::string> result;
	
	result.reserve(components1.size() + components2.size());

	result.insert(result.end(), components1.begin(), components1.end());
	result.insert(result.end(), components2.begin(), components2.end());

	/*for (size_t i = 0; i < components1.size(); ++i)
		result.push_back(components1[i]);

	for (size_t i = 0; i < components2.size(); ++i)
		result.push_back(components2[i]);*/
	
	return result;
}

std::string Path::FlattenPathComponents(const std::vector<std::string>& components)
{
	std::string result;
	
	for (size_t i = 0; i < components.size(); ++i)
	{
		if (i != 0)
			result += '/';
		
		result += components[i];
	}
	
	return result;
}

std::vector<std::string> Path::GetPathComponents(const std::string& _path)
{
	std::string path = NormalizeSlashes(_path);
	
	return String::Split(path, '/');
}

char Path::SafeRead(const std::string& a_String, int a_Position)
{
	if (a_Position < 0 || a_Position >= (int)a_String.size())
		return 0;

	return a_String[a_Position];
}
