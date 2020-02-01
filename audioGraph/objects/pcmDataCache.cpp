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

#include "framework.h" // listFiles
#include "Log.h"
#include "Path.h" // GetExtension
#include "pcmDataCache.h"
#include "soundmix.h" // PcmData
#include "StringEx.h" // ToLower
#include "Timer.h"
#include <map>

struct PcmDataCache
{
	std::map<std::string, PcmData*> elems;

	void addPath(const char * path, const bool recurse, const bool stripPaths, const bool createCaches)
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
				
				// check if this is a duplicate element. this could happen if different folders contain
				// a file with the same name, due to stripping paths
				
				auto & elem = elems[name];
				
				if (elem != nullptr)
				{
					delete pcmData;
					pcmData = nullptr;
				}
				else
				{
					elem = pcmData;
				}
			}
		}
		
		const auto t2 = g_TimerRT.TimeUS_get();
		
		printf("loading PCM data from %s took %.2fms\n", path, (t2 - t1) / 1000.0);
	}

	void clear()
	{
		for (auto & i : elems)
		{
			delete i.second;
			i.second = nullptr;
		}
		
		elems.clear();
	}

	const PcmData * get(const char * filename) const
	{
		const std::string filenameLower = String::ToLower(filename);
		
		auto i = elems.find(filenameLower);
		
		if (i == elems.end())
		{
			return nullptr;
		}
		else
		{
			return i->second;
		}
	}
};

static PcmDataCache s_pcmDataCache;

void fillPcmDataCache(const char * path, const bool recurse, const bool stripPaths, const bool createCaches)
{
	s_pcmDataCache.addPath(path, recurse, stripPaths, createCaches);
}

void clearPcmDataCache()
{
	s_pcmDataCache.clear();
}

const PcmData * getPcmData(const char * filename)
{
	return s_pcmDataCache.get(filename);
}
