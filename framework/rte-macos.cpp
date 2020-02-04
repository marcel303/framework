#include "Debugging.h"
#include "Log.h"
#include "rte-macos.h"

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
				
				printf("%d: %x, %s\n", (int)evIds[i], (int)evFlags[i], path);
				
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
