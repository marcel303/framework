/*
	Copyright (C) 2017 Marcel Smit
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

static void handleFileChange(const std::string & filename)
{
	const std::string extension = Path::GetExtension(filename, true);

	if (extension == "vs")
	{
		for (auto & i : g_shaderCache.m_map)
		{
			ShaderCacheElem & elem = i.second;

			if (elem.vs == filename)
				elem.reload();
		}
	}
	else if (extension == "ps")
	{
		for (auto & i : g_shaderCache.m_map)
		{
			ShaderCacheElem & elem = i.second;

			if (elem.ps == filename)
				elem.reload();
		}
	}
	else if (extension == "cs")
	{
		for (auto & i : g_computeShaderCache.m_map)
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
	
	// call real time editing callback
	
	if (framework.realTimeEditCallback)
	{
		framework.realTimeEditCallback(filename);
	}
}

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

				if (String::EndsWith(file, ".vs") || String::EndsWith(file, ".ps") || String::EndsWith(file, ".cs") || String::EndsWith(file, ".xml") || String::EndsWith(file, ".png") || String::EndsWith(file, ".jpg"))
					s_fileInfos.push_back(fi);
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

#elif defined(MACOS)

#include <CoreServices/CoreServices.h>

static FSEventStreamRef stream = nullptr;

static bool anyChanges = false;

static void callback(
	ConstFSEventStreamRef stream,
	void * callbackInfo,
	size_t numEvents,
	void * evPaths,
	const FSEventStreamEventFlags evFlags[],
	const FSEventStreamEventId evIds[])
{
	anyChanges = true;
	
	//

#if 0
	const char ** paths = (const char **)evPaths;
	
	for (int i = 0; i < numEvents; ++i)
	{
		printf("%d: %x, %s\n", (int)evIds[i], (int)evFlags[i], paths[i]);
	}
#endif
}

void initRealTimeEditing()
{
	Assert(stream == nullptr);
	
	fillFileInfos();
	
	const CFStringRef arg = CFStringCreateWithCString(
    	kCFAllocatorDefault,
    	".",
    	kCFStringEncodingUTF8);
	
	const CFArrayRef paths = CFArrayCreate(nullptr, (const void**)&arg, 1, nullptr);
	
	const CFAbsoluteTime latency = 0.1;

	stream = FSEventStreamCreate(
		NULL,
		&callback,
		NULL,
		paths,
		kFSEventStreamEventIdSinceNow,
		latency,
		kFSEventStreamCreateFlagNone);
	
	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	FSEventStreamStart(stream);
}

void shutRealTimeEditing()
{
	if (stream != nullptr)
	{
		FSEventStreamStop(stream);
		FSEventStreamInvalidate(stream);
		FSEventStreamRelease(stream);
		
		stream = nullptr;
	}
	
	clearFileInfos();
}

void tickRealTimeEditing()
{
	if (anyChanges)
	{
		anyChanges = false;
		
		checkFileInfos();
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
	// todo : add something similar to watch api on platforms other than win32 or osx
	
	static int x = 0;
	x++;
	
	if ((x % 60) != 0)
		return;
	
	checkFileInfos();
}

#endif
