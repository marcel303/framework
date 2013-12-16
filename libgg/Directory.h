#pragma once

#include <boost/filesystem/operations.hpp>
#include <string>
#include <vector>
#include "StringEx.h"

#include "Debugging.h"

using namespace boost::filesystem;

class Directory
{
public:
	static std::vector<std::string> GetDirectories(const std::string& path)
	{
		std::vector<std::string> result;

		boost::filesystem::path _path(path);

		directory_iterator dir(_path);
		directory_iterator end;

		for(; dir != end; ++dir)
		{
			if (is_directory(*dir))
			{
			#if 0//defined(WIN32) && !defined(GCC)
				result.push_back(dir->path().filename());
			#else
				result.push_back(dir->path().filename().string());
			#endif
			}
		}

		return result;
	}

	static std::vector<std::string> GetFiles(const std::string& path, const std::string& pattern, bool recursive)
	{
		Assert(pattern.empty());
		Assert(recursive == false);

		// todo: pattern support
		// todo: recurse

		std::vector<std::string> result;

		boost::filesystem::path _path(path);

		directory_iterator dir(_path);
		directory_iterator end;

		for(; dir != end; ++dir)
		{
			if (is_regular_file(*dir))
			{
			#if 0//defined(WIN32) && !defined(GCC)
				result.push_back(dir->path().filename());
			#else
				result.push_back(dir->path().filename().string());
			#endif
			}
		}

		return result;
	}

	static std::vector<std::string> GetFiles(const std::string& path, const std::string& pattern)
	{
		// todo: filter by pattern

		return GetFiles(path, pattern, false);
	}

	static std::vector<std::string> GetFiles(const std::string& path)
	{
		return GetFiles(path, "");
	}
};
