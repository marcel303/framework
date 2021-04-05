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

#include "audioThreading.h"
#include "audioTypeDB.h"
#include "audioTypes.h"
#include "graph.h"
#include <atomic>
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
struct AudioGraphContext;
struct AudioGraphManager;
struct AudioNodeBase;
struct AudioOutputChannel;
struct AudioPlug;
struct AudioSource;
struct AudioVoice;
struct AudioVoiceManager;

extern AUDIO_THREAD_LOCAL AudioGraph * g_currentAudioGraph;

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
		float value1_mainThread;
		float value2_mainThread;
		float value3_mainThread;
		float value4_mainThread;
		
		float value1_audioThread;
		float value2_audioThread;
		float value3_audioThread;
		float value4_audioThread;
		
		Memf()
			: value1_mainThread(0.f)
			, value2_mainThread(0.f)
			, value3_mainThread(0.f)
			, value4_mainThread(0.f)
		{
		}
		
		void syncMainToAudio()
		{
			value1_audioThread = value1_mainThread;
			value2_audioThread = value2_mainThread;
			value3_audioThread = value3_mainThread;
			value4_audioThread = value4_mainThread;
		}
	};
	
	struct Mems
	{
		std::string value_mainThread;
		
		std::string value_audioThread;
		
		void syncMainToAudio()
		{
			value_audioThread = value_mainThread;
		}
	};
	
	std::atomic<bool> isPaused;
	std::atomic<bool> rampDownRequested;
	bool rampDown;
	std::atomic<bool> rampedDown;
	
	std::map<GraphNodeId, AudioNodeBase*> nodes;
	
	int currentTickTraversalId;
	
#if AUDIO_GRAPH_ENABLE_TIMING
	Graph * graph;
#endif
	
	std::vector<ValueToFree> valuesToFree;
	
	std::set<AudioVoice*> audioVoices;
	
	double time;
	
	std::map<std::string, Memf> memf; // registered float type memory
	std::map<std::string, Mems> mems; // registered string type memory
	
	std::vector<AudioControlValue> controlValues; // registered control values
	
	std::set<std::string> activeFlags; // flags to communicate state between main <-> audio threads
	
	std::vector<AudioEvent> events; // registered events
	std::set<std::string> triggeredEvents_mainThread;  // events triggered from the main thread (authorative)
	std::set<std::string> triggeredEvents_audioThread; // triggered events as captured from the authorative copy
	
	AudioGraphContext * context; 
	
	AudioGraph(AudioGraphContext * context, const bool isPaused);
	~AudioGraph();
	
	void destroy();
	void connectToInputLiteral(AudioPlug & input, const std::string & inputValue);
	
	bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex);
	void freeVoice(AudioVoice *& voice);
	
	// called from the main thread
	void lockControlValues();
	void unlockControlValues();
	
	// called from the audio thread
	void syncMainToAudio(const float dt); // synchronize control values and other state from the main thread to the audio thread
	void tickAudio(const float dt, const bool syncMainToAudio);
	
	// called from the main & audio thread
	void setFlag(const char * name, const bool value = true);
	void resetFlag(const char * name);
	// called from the audio thread
	bool isFlagSet(const char * name) const;
	
	// called from any thread
	void registerMemf(const char * name, const float value1, const float value2, const float value3, const float value4);
	void registerMems(const char * name, const char * value);
	void registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY);
	void unregisterControlValue(const char * name);
	
	// called from any thread
	void registerEvent(const char * name);
	void unregisterEvent(const char * name);
	bool isEventTriggered(const char * name, const bool isMainThread);
	
	// called from the main & audio thread
	void setMemf(const char * name, const float value1, const float value2 = 0.f, const float value3 = 0.f, const float value4 = 0.f);
	Memf getMemf(const char * name, const bool isMainThread) const;
	
	// called from the main & audio thread
	void setMems(const char * name, const char * value);
	void getMems(const char * name, std::string & result) const;
	
	// called from the main thread
	void triggerEvent(const char * event);
};

//

void pushAudioGraph(AudioGraph * audioGraph);
void popAudioGraph();

//

AudioNodeBase * createAudioNode(
	const GraphNodeId nodeId,
	const std::string & typeName);

AudioGraph * constructAudioGraph(
	const Graph & graph,
	const Graph_TypeDefinitionLibrary * typeDefinitionLibrary,
	AudioGraphContext * context,
	const bool createdPaused);
