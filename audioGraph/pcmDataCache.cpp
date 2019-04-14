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

#include "framework.h" // listFiles
#include "Log.h"
#include "Path.h"
#include "pcmDataCache.h"
#include "soundmix.h" // PcmData
#include "StringEx.h"
#include "Timer.h"
#include <map>

static std::map<std::string, PcmData*> s_pcmDataCache;

void fillPcmDataCache(const char * path, const bool recurse, const bool stripPaths, const bool createCaches)
{
	LOG_DBG("filling PCM data cache with path: %s", path);
	
	const auto t1 = g_TimerRT.TimeUS_get();
	
	const auto filenames = listFiles(path, recurse);
	
	for (auto & filename : filenames)
	{
		const auto extension = Path::GetExtension(filename, true);
		
		if (extension == "cache")
			continue;
		
		if (extension != "wav" && extension != "ogg")
			continue;
		
		const std::string filenameLower = String::ToLower(filename);
		
		PcmData * pcmData = new PcmData();
		
		if (pcmData->load(filenameLower.c_str(), 0, createCaches) == false)
		{
			delete pcmData;
			pcmData = nullptr;
		}
		else
		{
			const std::string name = stripPaths ? Path::GetFileName(filenameLower) : filenameLower;
			
			auto & elem = s_pcmDataCache[name];
			
			delete elem;
			elem = nullptr;
			
			elem = pcmData;
		}
	}
	
	const auto t2 = g_TimerRT.TimeUS_get();
	
	printf("loading PCM data from %s took %.2fms\n", path, (t2 - t1) / 1000.0);
}

void clearPcmDataCache()
{
	for (auto & i : s_pcmDataCache)
	{
		delete i.second;
		i.second = nullptr;
	}
	
	s_pcmDataCache.clear();
}

const PcmData * getPcmData(const char * filename)
{
	const std::string filenameLower = String::ToLower(filename);
	
	auto i = s_pcmDataCache.find(filenameLower);
	
	if (i == s_pcmDataCache.end())
	{
		return nullptr;
	}
	else
	{
		return i->second;
	}
}
