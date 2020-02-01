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

#include "binaural.h"
#include "binaural_cipic.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"
#include "hrirSampleSetCache.h"
#include <map>

static std::map<std::string, binaural::HRIRSampleSet*> s_hrirSampleSetCache;

void fillHrirSampleSetCache(const char * path, const char * name, const HRIRSampleSetType type)
{
	const auto paths = std::vector<std::string>({ path });
	
	for (auto & path : paths)
	{
		binaural::HRIRSampleSet * sampleSet = new binaural::HRIRSampleSet();
		
		bool result = false;
		
		switch (type)
		{
		case kHRIRSampleSetType_Cipic:
			result = binaural::loadHRIRSampleSet_Cipic(path.c_str(), *sampleSet);
			break;
		case kHRIRSampleSetType_Ircam:
			result = binaural::loadHRIRSampleSet_Ircam(path.c_str(), *sampleSet);
			break;
		case kHRIRSampleSetType_Mit:
			result = binaural::loadHRIRSampleSet_Mit(path.c_str(), *sampleSet);
			break;
		}
		
		if (result == false)
		{
			delete sampleSet;
			sampleSet = nullptr;
		}
		else
		{
			sampleSet->finalize();
			
			auto & elem = s_hrirSampleSetCache[name];
			
			delete elem;
			elem = nullptr;
			
			elem = sampleSet;
		}
	}
}

void clearHrirSampleSetCache()
{
	for (auto & i : s_hrirSampleSetCache)
	{
		delete i.second;
		i.second = nullptr;
	}
	
	s_hrirSampleSetCache.clear();
}

const binaural::HRIRSampleSet * getHrirSampleSet(const char * name)
{
	auto i = s_hrirSampleSetCache.find(name);
	
	if (i == s_hrirSampleSetCache.end())
	{
		return nullptr;
	}
	else
	{
		return i->second;
	}
}
