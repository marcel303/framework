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

#pragma once

#include <functional>
#include <string>
#include <vector>

struct rteFileWatcherBase
{
	std::function<void(const char*)> fileChanged;
	
	virtual ~rteFileWatcherBase() { }
	
	virtual void init(const char * path) = 0;
	virtual void shut() = 0;
	virtual void tick() = 0;
};

struct rteFileInfo
{
	std::string filename;
	time_t time;
};

struct rteFileWatcher_Basic : rteFileWatcherBase
{
	std::vector<rteFileInfo> fileInfos;
	
	std::function<void(const char*)> fileChanged;

	virtual void init(const char * path) override;
	virtual void shut() override;
	virtual void tick() override;
};

#if defined(MACOS)

#include <CoreServices/CoreServices.h>
#include <string>

struct rteFileWatcher_OSX : rteFileWatcherBase
{
	std::string path;
	
	FSEventStreamRef stream = nullptr;

	virtual ~rteFileWatcher_OSX() override;
	
	virtual void init(const char * path) override;
	virtual void shut() override;
	virtual void tick() override;
};

#endif

#if defined(WINDOWS)

// todo : remove Windows.h header file include
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct rteFileWatcher_Windows
{
	std::string path;
	
	HANDLE fileWatcher = INVALID_HANDLE_VALUE;
	
	std::function<void(const char*)> fileChanged;

	~rteFileWatcher_OSX();
	
	void init(const char * path);
	void shut();
};

#endif
