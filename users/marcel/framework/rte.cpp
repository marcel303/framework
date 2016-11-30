#include "Path.h"
#include "internal.h"
#include "rte.h"
#include "StringEx.h"
#include <sys/stat.h>
#include <Windows.h>

#if defined(WIN32)

static void handleFileChange(const std::string & filename)
{
	const std::string extension = Path::GetExtension(filename);

	if (extension == "vs")
	{
		for (auto i : g_shaderCache.m_map)
		{
			ShaderCacheElem & elem = i.second;

			if (elem.vs == filename)
				elem.reload();
		}
	}
	else if (extension == "ps")
	{
		for (auto i : g_shaderCache.m_map)
		{
			ShaderCacheElem & elem = i.second;

			if (elem.ps == filename)
				elem.reload();
		}
	}
	else if (extension == "cs")
	{
		for (auto i : g_computeShaderCache.m_map)
		{
			ComputeShaderCacheElem & elem = i.second;

			if (elem.name == filename)
				elem.reload();
		}
	}
	else if (extension == "inc")
	{
		clearCaches(CACHE_SHADER);
	}
	else if (extension == "png" || extension == "jpg")
	{
		Sprite(filename.c_str()).reload();
	}
	else
	{
		// todo : call real time editing callback
	}
}

//

struct FileInfo
{
	std::string filename;
	time_t time;
};

static std::vector<FileInfo> s_fileInfos;

static HANDLE s_fileWatcher = INVALID_HANDLE_VALUE;

void initRealTimeEditing()
{
	s_fileInfos.clear();

	if (s_fileWatcher != INVALID_HANDLE_VALUE)
	{
		BOOL result = FindCloseChangeNotification(s_fileWatcher);
		Assert(result);

		s_fileWatcher = INVALID_HANDLE_VALUE;
	}

	std::vector<std::string> files = listFiles(".", true);

	for (auto & file : files)
	{
		FILE * f = fopen(file.c_str(), "rb");
		if (f)
		{
			struct _stat s;
			if (_fstat(fileno(f), &s) == 0)
			{
				FileInfo fi;
				fi.filename = file;
				fi.time = s.st_mtime;

				if (String::EndsWith(file, ".vs") || String::EndsWith(file, ".ps") || String::EndsWith(file, ".cs") || String::EndsWith(file, ".xml") || String::EndsWith(file, ".png") || String::EndsWith(file, ".jpg"))
					s_fileInfos.push_back(fi);
			}

			fclose(f);
			f = 0;
		}
	}

	Assert(s_fileWatcher == INVALID_HANDLE_VALUE);
	s_fileWatcher = FindFirstChangeNotificationA(".", TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	Assert(s_fileWatcher != INVALID_HANDLE_VALUE);
	if (s_fileWatcher == INVALID_HANDLE_VALUE)
		logError("failed to find first change notification");
}

void shutRealTimeEditing()
{
	s_fileInfos.clear();
}

void tickRealTimeEditing()
{
	if (s_fileWatcher != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(s_fileWatcher, 0) != WAIT_OBJECT_0)
		{
			return;
		}

		Sleep(100);
	}

	for (auto & fi: s_fileInfos)
	{
		FILE * f = fopen(fi.filename.c_str(), "rb");

		if (f)
		{
			bool changed = false;

			struct _stat s;
			if (_fstat(fileno(f), &s) == 0)
			{
				if (fi.time < s.st_mtime)
				{
					// file has changed!

					logDebug("%s has changed!", fi.filename.c_str());

					fi.time = s.st_mtime;

					changed = true;
				}
			}

			fclose(f);
			f = 0;

			if (changed)
			{
				handleFileChange(fi.filename);
			}
		}
	}

	if (s_fileWatcher != INVALID_HANDLE_VALUE)
	{
		BOOL result = FindNextChangeNotification(s_fileWatcher);
		Assert(result);

		if (!result)
		{
			logError("failed to watch for next file change notification");
		}
	}
}

#else

void initRealTimeEditing()
{
}

void shutRealTimeEditing()
{
}

void tickRealTimeEditing()
{
}

#endif
