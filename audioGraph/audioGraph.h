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

#pragma once

#include "audioTypes.h"
#include "graph.h"
#include <map>
#include <set>
#include <string>
#include <vector>

#if defined(DEBUG)
	#define AUDIO_GRAPH_ENABLE_TIMING 0
#else
	#define AUDIO_GRAPH_ENABLE_TIMING 0
#endif

struct AudioBuffer;
struct AudioGraph;
struct AudioNodeBase;
struct AudioOutputChannel;
struct AudioPlug;

struct SDL_mutex;

extern __thread AudioGraph * g_currentAudioGraph;

extern double g_currentAudioTime;

struct AudioGraph
{
	struct ValueToFree
	{
		enum Type
		{
			kType_Unknown,
			kType_Bool,
			kType_Int,
			kType_Float,
			kType_String,
			kType_AudioValue
		};
		
		Type type;
		void * mem;
		
		ValueToFree()
			: type(kType_Unknown)
			, mem(nullptr)
		{
		}
		
		ValueToFree(const Type _type, void * _mem)
			: type(_type)
			, mem(_mem)
		{
		}
	};
	
	struct Memf
	{
		float value1;
		float value2;
		float value3;
		float value4;
		
		Memf()
			: value1(0.f)
			, value2(0.f)
			, value3(0.f)
			, value4(0.f)
		{
		}
	};
	
	struct Mems
	{
		std::string value;
	};
	
	std::map<GraphNodeId, AudioNodeBase*> nodes;
	
	mutable int nextTickTraversalId;
	
	Graph * graph; // todo : remove ?
	
	std::vector<ValueToFree> valuesToFree;
	
	double time;
	
	std::set<std::string> activeFlags;
	std::map<std::string, Memf> memf;
	std::map<std::string, Mems> mems;
	std::vector<std::string> events;
	
	std::vector<AudioControlValue> controlValues;
	
	SDL_mutex * mutex;
	
	AudioGraph();
	~AudioGraph();
	
	void destroy();
	void connectToInputLiteral(AudioPlug & input, const std::string & inputValue);
	
	void tick(const float dt);
	
	// called from any thread
	void setFlag(const char * name, const bool value = true);
	void resetFlag(const char * name);
	bool isFLagSet(const char * name) const;
	
	// called from any thread
	void registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY);
	void unregisterControlValue(const char * name);
	bool findControlValue(const char * name, AudioControlValue & result) const;
	void exportControlValues();
	
	// called from any thread
	void setMemf(const char * name, const float value1, const float value2 = 0.f, const float value3 = 0.f, const float value4 = 0.f);
	Memf getMemf(const char * name) const;
	
	void setMems(const char * name, const char * value);
	Mems getMems(const char * name) const;
	
	void triggerEvent(const char * event);
};

//

AudioNodeBase * createAudioNode(const GraphNodeId nodeId, const std::string & typeName, AudioGraph * audioGraph);

AudioGraph * constructAudioGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);

//

struct PcmData;

void fillPcmDataCache(const char * path, const bool recurse, const bool stripPaths);
void clearPcmDataCache();
const PcmData * getPcmData(const char * filename);

//

namespace binaural
{
	struct HRIRSampleSet;
}

enum HRIRSampleSetType
{
	kHRIRSampleSetType_Cipic,
	kHRIRSampleSetType_Ircam,
	kHRIRSampleSetType_Mit
};

void fillHrirSampleSetCache(const char * path, const char * name, const HRIRSampleSetType type);
void clearHrirSampleSetCache();
const binaural::HRIRSampleSet * getHrirSampleSet(const char * name);
