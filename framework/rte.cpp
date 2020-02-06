/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Path.h"
#include "internal.h"
#include "rte.h"
#include "StringEx.h"
#include <sys/stat.h>

#if defined(WIN32)
    #include <Windows.h>
#endif

// todo : windows,linux : add file watchers for multiple resource paths

static void handleFileChange(const char * filename)
{
	const std::string extension = Path::GetExtension(filename, true);

	if (extension == "vs" || extension == "txt")
	{
		g_shaderCache.handleSourceChanged(filename);
	}
	else if (extension == "ps" || extension == "txt")
	{
		g_shaderCache.handleSourceChanged(filename);
	}
#if ENABLE_OPENGL && ENABLE_OPENGL_COMPUTE_SHADER // todo : enable for vs and ps for metal
	else if (extension == "cs" || extension == "txt")
	{
		for (auto & i : g_computeShaderCache.m_map)
		{
			ComputeShaderCacheElem & elem = i.second;

			if (elem.name == filename)
				elem.reload();
		}
	}
#endif
	else if (extension == "inc")
	{
		clearCaches(CACHE_SHADER);
	}
	else if (extension == "png" || extension == "jpg")
	{
		Sprite(filename).reload();
	}
	
	// call real time editing callback
	
	if (framework.realTimeEditCallback)
	{
		framework.realTimeEditCallback(filename);
	}
	
	framework.changedFiles.push_back(filename);
}

#if defined(MACOS)

#include "rte-macos.h"
#include <list>

std::list<rteFileWatcher_OSX*> s_fileWatchers;

void initRealTimeEditing()
{
	for (auto & resourcePath : framework.resourcePaths)
	{
		rteFileWatcher_OSX * fileWatcher = new rteFileWatcher_OSX();
		
		fileWatcher->init(resourcePath.c_str());
		fileWatcher->fileChanged = handleFileChange;
		
		s_fileWatchers.push_back(fileWatcher);
	}
}

void shutRealTimeEditing()
{
	for (auto *& fileWatcher : s_fileWatchers)
	{
		fileWatcher->shut();
		
		delete fileWatcher;
		fileWatcher = nullptr;
	}
	
	s_fileWatchers.clear();
}

void tickRealTimeEditing()
{
	// tod
}

#else

#if 1

struct RTEFileInfo
{
	std::string filename;
	time_t time;
};

static std::vector<RTEFileInfo> s_fileInfos;

static void fillFileInfos()
{
	s_fileInfos.clear();

	std::vector<std::string> files = listFiles(".", true);

	for (auto & file : files)
	{
		FILE * f = fopen(file.c_str(), "rb");
		if (f)
		{
			struct stat s;
			if (fstat(fileno(f), &s) == 0)
			{
				RTEFileInfo fi;
				fi.filename = file;
				fi.time = s.st_mtime;

			#if 0 // note : we want to track all files now, to ensure Framework::fileHasChanged works as expected and not just for a subset of files
				if (String::EndsWith(file, ".vs") || String::EndsWith(file, ".ps") || String::EndsWith(file, ".cs") || String::EndsWith(file, ".xml") || String::EndsWith(file, ".txt") ||
					String::EndsWith(file, ".png") || String::EndsWith(file, ".jpg"))
				{
					s_fileInfos.push_back(fi);
				}
			#else
				s_fileInfos.push_back(fi);
			#endif
			}

			fclose(f);
			f = 0;
		}
	}
}

static void clearFileInfos()
{
	s_fileInfos.clear();
}

static void checkFileInfos()
{
	for (auto & fi: s_fileInfos)
	{
		FILE * f = fopen(fi.filename.c_str(), "rb");

		if (f)
		{
			bool changed = false;

			struct stat s;
			if (fstat(fileno(f), &s) == 0)
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
}

#endif

#if defined(WIN32)

static HANDLE s_fileWatcher = INVALID_HANDLE_VALUE;

void initRealTimeEditing()
{
	if (s_fileWatcher != INVALID_HANDLE_VALUE)
	{
		BOOL result = FindCloseChangeNotification(s_fileWatcher);
		Assert(result);

		s_fileWatcher = INVALID_HANDLE_VALUE;
	}
	
	fillFileInfos();
	
	Assert(s_fileWatcher == INVALID_HANDLE_VALUE);
	s_fileWatcher = FindFirstChangeNotificationA(".", TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	Assert(s_fileWatcher != INVALID_HANDLE_VALUE);
	if (s_fileWatcher == INVALID_HANDLE_VALUE)
		logError("failed to find first change notification");
}

void shutRealTimeEditing()
{
	clearFileInfos();
	
	BOOL result = FindCloseChangeNotification(fileWatcher);
	Assert(result);
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
	
	checkFileInfos();
	
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
	fillFileInfos();
}

void shutRealTimeEditing()
{
	clearFileInfos();
}

void tickRealTimeEditing()
{
	static int x = 0;
	
	if (x == 0)
	{
		logWarning("using non-optimized code path for checking for file changes. this could be hefty and cause periodic stutters when there's lots of files!");
	}
	
	x++;
	
	if ((x % 60) != 0)
		return;
	
	checkFileInfos();
}

#endif

#endif
