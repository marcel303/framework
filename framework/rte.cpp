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
#include "rte-filewatcher.h"
#include "StringEx.h"
#include <sys/stat.h>

#if defined(WIN32)
    #include <Windows.h>
#endif

/*

RTE strategies:

	OSX: create a file watcher for each resource path. get notified of each individual file changed. invoke change handler
	Windows: create a file watcher for each resource path. get notified when a file inside a path is changed. check file infos to see which files have changed. invoke change handler
	Linux: create a basic file wachter for each resource path. check the time stamps of each file for each file watcher to check for modification time stamps. invoke change handler

*/

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
#if ENABLE_COMPUTE_SHADER
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

//

void rteFileWatcher_Basic::init(const char * path)
{
	shut();

	//

	this->path = path;

	std::vector<std::string> files = listFiles(path, true);

	for (auto & file : files)
	{
		FILE * f = fopen(file.c_str(), "rb");
		
		if (f != nullptr)
		{
			struct stat s;
			if (fstat(fileno(f), &s) == 0)
			{
				rteFileInfo fi;
				fi.filename = file;
				fi.time = s.st_mtime;

				fileInfos.push_back(fi);
			}

			fclose(f);
			f = nullptr;
		}
	}
}

void rteFileWatcher_Basic::shut()
{
	fileInfos.clear();
}

void rteFileWatcher_Basic::tick()
{
	for (auto & fi: fileInfos)
	{
		FILE * f = fopen(fi.filename.c_str(), "rb");

		if (f != nullptr)
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
			f = nullptr;

			if (changed)
			{
				const char * path = fi.filename.c_str();

				if (strcasestr(path, this->path.c_str()) == path)
				{
					path += this->path.size();
					if (path[0] == '/')
						path++;

					if (fileChanged != nullptr)
					{
						fileChanged(path);
					}
				}
			}
		}
	}
}

//

struct rteFileWatcher_BasicWithPathOptimize : rteFileWatcherBase
{
	rteFileWatcherBase * pathWatcher = nullptr;
	rteFileWatcher_Basic fileWatcher_Basic;
	bool anyChange = false;

	virtual void init(const char * path) override
	{
		fileWatcher_Basic.init(path);
		fileWatcher_Basic.fileChanged = [](const char * filename)
		{
			handleFileChange(filename);
		};

	#if defined(WINDOWS)
		pathWatcher = new rteFileWatcher_Windows();
	#endif
		
		if (pathWatcher != nullptr)
		{
			pathWatcher->init(path);

			pathWatcher->pathChanged = [&](const char * path)
			{
				anyChange = true;
			};
		}
	}

	virtual void shut() override
	{
		if (pathWatcher != nullptr)
		{
			pathWatcher->shut();

			delete pathWatcher;
			pathWatcher = nullptr;
		}

		fileWatcher_Basic.shut();
	}

	virtual void tick() override
	{
		pathWatcher->tick();

		if (anyChange)
		{
			anyChange = false;

		#if defined(WINDOWS)
			Sleep(100);
		#endif
		
			fileWatcher_Basic.tick();
		}
	}
};

//

#if defined(MACOS) || defined(WINDOWS)

#include <list>

std::list<rteFileWatcherBase*> s_fileWatchers;

void initRealTimeEditing()
{
	for (auto & resourcePath : framework.resourcePaths)
	{
	#if defined(MACOS)
		rteFileWatcher_OSX * fileWatcher = new rteFileWatcher_OSX();
	#elif defined(WINDOWS)
		rteFileWatcher_BasicWithPathOptimize * fileWatcher = new rteFileWatcher_BasicWithPathOptimize();
	#else
		#error
	#endif
		
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
	for (auto * fileWatcher : s_fileWatchers)
	{
		fileWatcher->tick();
	}
}

#elif defined(WIN32)

static rteFileWatcher_BasicWithPathOptimize s_fileWatcher;

void initRealTimeEditing()
{
	s_fileWatcher.init(".");
}

void shutRealTimeEditing()
{
	s_fileWatcher.shut();
}

void tickRealTimeEditing()
{
	s_fileWatcher.tick();
}

#else

static rteFileWatcher_Basic s_fileWatcher;

void initRealTimeEditing()
{
	s_fileWatcher.init(".");
}

void shutRealTimeEditing()
{
	s_fileWatcher.shut();
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
	
	s_fileWatcher.tick();
}

#endif
