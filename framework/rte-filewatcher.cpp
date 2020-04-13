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

#include "rte-filewatcher.h"

#if defined(MACOS)

#include "Debugging.h"
#include "Log.h"

rteFileWatcher_OSX::~rteFileWatcher_OSX()
{
	Assert(stream == nullptr);
}

static void callback(
	ConstFSEventStreamRef stream,
	void * callbackInfo,
	size_t numEvents,
	void * evPaths,
	const FSEventStreamEventFlags evFlags[],
	const FSEventStreamEventId evIds[])
{
	rteFileWatcher_OSX * self = (rteFileWatcher_OSX*)callbackInfo;
	
	const char ** paths = (const char **)evPaths;
	
	for (int i = 0; i < numEvents; ++i)
	{
		if ((evFlags[i] & kFSEventStreamEventFlagItemIsFile) == 0)
			continue;
		
		const FSEventStreamEventFlags flagsToCheckFor =
			kFSEventStreamEventFlagItemModified |
			kFSEventStreamEventFlagItemCreated |
			kFSEventStreamEventFlagItemRemoved |
			kFSEventStreamEventFlagItemRenamed;
	
		if ((evFlags[i] & flagsToCheckFor) != 0)
		{
			const char * path = paths[i];
		
			if (strcasestr(path, self->path.c_str()) == path)
			{
				path += self->path.size();
				if (path[0] == '/')
					path++;
				
				//printf("%d: %x, %s\n", (int)evIds[i], (int)evFlags[i], path);
				
				if (self->fileChanged != nullptr)
				{
					self->fileChanged(path);
				}
			}
			else
			{
				LOG_WRN("unable to translate absolute path into relative path", path);
			}
		}
	}
}

void rteFileWatcher_OSX::init(const char * in_path)
{
	Assert(stream == nullptr);
	
	path = in_path;
	
	const CFStringRef arg = CFStringCreateWithCString(
    	kCFAllocatorDefault,
    	path.c_str(),
    	kCFStringEncodingUTF8);
	
	const CFArrayRef paths = CFArrayCreate(nullptr, (const void**)&arg, 1, nullptr);
	
	const CFAbsoluteTime latency = 0.1;

	FSEventStreamContext context;
	context.version = 0;
	context.info = this;
	context.retain = nullptr;
	context.release = nullptr;
	context.copyDescription = nullptr;
	
	stream = FSEventStreamCreate(
		NULL,
		&callback,
		&context,
		paths,
		kFSEventStreamEventIdSinceNow,
		latency,
		kFSEventStreamCreateFlagFileEvents);
	
	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	FSEventStreamStart(stream);
}

void rteFileWatcher_OSX::shut()
{
	if (stream != nullptr)
	{
		FSEventStreamStop(stream);
		FSEventStreamInvalidate(stream);
		FSEventStreamRelease(stream);
		
		stream = nullptr;
	}
}

void rteFileWatcher_OSX::tick()
{
}

#endif

#if defined(WINDOWS)

#include "Debugging.h"
#include "Log.h"

rteFileWatcher_Windows::~rteFileWatcher_Windows()
{
	Assert(fileWatcher == INVALID_HANDLE_VALUE);
}

void rteFileWatcher_Windows::init(const char * path)
{
	this->path = path;

	Assert(fileWatcher == INVALID_HANDLE_VALUE);
	fileWatcher = FindFirstChangeNotificationA(path, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	Assert(fileWatcher != INVALID_HANDLE_VALUE);
	if (fileWatcher == INVALID_HANDLE_VALUE)
		LOG_ERR("failed to find first change notification. path=%s", path);
}

void rteFileWatcher_Windows::shut()
{
	BOOL result = FindCloseChangeNotification(fileWatcher);
	Assert(result);

	fileWatcher = INVALID_HANDLE_VALUE;
}

void rteFileWatcher_Windows::tick()
{
	if (fileWatcher != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(fileWatcher, 0) == WAIT_OBJECT_0)
		{
			if (pathChanged != nullptr)
				pathChanged(path.c_str());

			BOOL result = FindNextChangeNotification(fileWatcher);
			Assert(result);

			if (!result)
			{
				LOG_ERR("failed to watch for next file change notification", 0);
			}
		}
	}
}

#endif
