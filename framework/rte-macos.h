#pragma once

#include <CoreServices/CoreServices.h>
#include <functional>
#include <string>

struct rteFileWatcher_OSX
{
	std::string path;
	
	FSEventStreamRef stream = nullptr;
	
	std::function<void(const char*)> fileChanged;

	~rteFileWatcher_OSX();
	
	void init(const char * path);
	void shut();
};
