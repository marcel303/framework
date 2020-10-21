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
#include "audioTypes.h"
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

struct AudioGraph;
struct AudioGraphContext;
struct AudioGraphFileRTC;
struct AudioRealTimeConnection;
struct AudioValueHistorySet;
struct AudioVoiceManager;
struct Graph;
struct Graph_TypeDefinitionLibrary;
struct GraphEdit;

struct AudioGraphInstance
{
	AudioGraph * audioGraph;
	AudioRealTimeConnection * realTimeConnection;

	AudioGraphInstance();
	~AudioGraphInstance();
};

struct AudioGraphFile
{
	std::string filename;
	
	std::list<AudioGraphInstance*> instanceList;

	const AudioGraphInstance * activeInstance;

	AudioGraphFileRTC * realTimeConnection;
	
	AudioValueHistorySet * audioValueHistorySet;
	AudioValueHistorySet * audioValueHistorySetCapture;
	
	GraphEdit * graphEdit;

	AudioGraphFile();
	~AudioGraphFile();
};

struct AudioGraphManager
{
	virtual ~AudioGraphManager() { }
	
	// called from the app thread
	virtual AudioGraphContext * createContext(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr) = 0;
	virtual void freeContext(AudioGraphContext *& context) = 0;
	virtual AudioGraphContext * getContext() = 0;
	
	// called from the app thread
	virtual AudioGraphInstance * createInstance(const char * filename, AudioGraphContext * context = nullptr, const bool createdPaused = false) = 0;
	virtual void free(AudioGraphInstance *& instance, const bool doRampDown) = 0;
	virtual void tickMain() = 0;
	
	// called from the audio thread
	virtual void tickAudio(const float dt) = 0;
	virtual void tickVisualizers() = 0;
};

/*
Basic audio graph manager, with optional support for caching graphs on load. This graph
manager has less overhead than the real-time editing (RTE) and MultiRTE implementations,
as it doesn't support real-time editing and doesn't create editors for loaded graphs.
*/
struct AudioGraphManager_Basic : AudioGraphManager
{
	struct GraphCacheElem
	{
		bool isValid;
		Graph * graph;
		
		GraphCacheElem()
			: isValid(false)
			, graph(nullptr)
		{
		}
	};
	
	Graph_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::map<std::string, GraphCacheElem> graphCache;
	bool cacheOnCreate;
	
	std::list<AudioGraphInstance*> instances;
	
	AudioMutexBase * audioMutex;
	
	std::set<AudioGraphContext*> allocatedContexts;
	AudioGraphContext * context;
	
	AudioGraphManager_Basic(const bool cacheOnCreate);
	virtual ~AudioGraphManager_Basic() override;
	
	// called from the app thread
	void init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr);
	void shut();
	void addGraphToCache(const char * filename);
	
	// called from the app thread
	virtual AudioGraphContext * createContext(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr) override;
	virtual void freeContext(AudioGraphContext *& context) override;
	virtual AudioGraphContext * getContext() override;
	
	// called from the app thread
	virtual AudioGraphInstance * createInstance(const char * filename, AudioGraphContext * context = nullptr, const bool createdPaused = false) override;
	virtual void free(AudioGraphInstance *& instance, const bool doRampDown) override;
	virtual void tickMain() override;
	
	// called from the audio thread
	virtual void tickAudio(const float dt) override;
	virtual void tickVisualizers() override;
};

/*
Audio graph manager with real-time editing (RTE) support. RTE support means the audio
graph manager implements tickEditor and drawEditor, which allows it to present a graphical
user interface for editing audio graphs in real-time. The active graph being edited is
selected using selectFile or selectInstance. selectInstance also allows one to specify
the instance for which to show real-time information such as visualizers and timing data.
*/
struct AudioGraphManager_RTE : AudioGraphManager
{
	Graph_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::map<std::string, AudioGraphFile*> files;
	
	AudioGraphFile * selectedFile;
	
	AudioMutexBase * audioMutex;
	
	std::set<AudioGraphContext*> allocatedContexts;
	AudioGraphContext * context;
	
	int displaySx;
	int displaySy;
	
	AudioGraphManager_RTE(const int displaySx, const int displaySy);
	virtual ~AudioGraphManager_RTE() override;
	
	// called from the app thread
	void init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr);
	void shut();
	
	// called from the app thread
	void selectFile(const char * filename);
	void selectInstance(const AudioGraphInstance * instance);
	
	// called from the app thread
	virtual AudioGraphContext * createContext(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr) override;
	virtual void freeContext(AudioGraphContext *& context) override;
	virtual AudioGraphContext * getContext() override;
	
	// called from the app thread
	virtual AudioGraphInstance * createInstance(const char * filename, AudioGraphContext * context = nullptr, const bool createdPaused = false) override;
	virtual void free(AudioGraphInstance *& instance, const bool doRampDown) override;
	virtual void tickMain() override;
	
	// called from the audio thread
	virtual void tickAudio(const float dt) override;
	virtual void tickVisualizers() override;
	
	// called from the app thread
	bool tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured);
	void drawEditor(const int sx, const int sy);
};

//

struct AudioGraphManager_MultiRTE : AudioGraphManager
{
	Graph_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::map<std::string, AudioGraphFile*> files;
	
	AudioGraphFile * selectedFile;
	
	AudioMutexBase * audioMutex;
	
	std::set<AudioGraphContext*> allocatedContexts;
	AudioGraphContext * context;
	
	int displaySx;
	int displaySy;
	
	AudioGraphManager_MultiRTE(const int displaySx, const int displaySy);
	virtual ~AudioGraphManager_MultiRTE() override;
	
	// called from the app thread
	void init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr);
	void shut();
	
	// called from the app thread
	void selectFile(const char * filename);
	void selectInstance(const AudioGraphInstance * instance);
	
	// called from the app thread
	virtual AudioGraphContext * createContext(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr) override;
	virtual void freeContext(AudioGraphContext *& context) override;
	virtual AudioGraphContext * getContext() override;
	
	// called from the app thread
	virtual AudioGraphInstance * createInstance(const char * filename, AudioGraphContext * context = nullptr, const bool createdPaused = false) override;
	virtual void free(AudioGraphInstance *& instance, const bool doRampDown) override;
	virtual void tickMain() override;
	
	// called from the audio thread
	virtual void tickAudio(const float dt) override;
	virtual void tickVisualizers() override;
	
	// called from the app thread
	bool tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured);
	void drawEditor(const int sx, const int sy);
};
