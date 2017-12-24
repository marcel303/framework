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
#include <list>
#include <map>
#include <string>
#include <vector>

struct AudioGraph;
struct AudioGraphFileRTC;
struct AudioGraphGlobals;
struct AudioRealTimeConnection;
struct AudioValueHistorySet;
struct Graph;
struct GraphEdit;
struct GraphEdit_TypeDefinitionLibrary;

struct SDL_mutex;

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
	
	std::list<AudioGraphInstance> instanceList;

	const AudioGraphInstance * activeInstance;

	AudioGraphFileRTC * realTimeConnection;
	
	GraphEdit * graphEdit;

	AudioGraphFile();
	~AudioGraphFile();
};

struct AudioGraphManager
{
	virtual ~AudioGraphManager() { }
	
	// called from the app thread
	virtual AudioGraphInstance * createInstance(const char * filename) = 0;
	virtual void free(AudioGraphInstance *& instance) = 0;
	
	// called from the audio thread
	virtual void tick(const float dt) = 0;
	virtual void tickVisualizers() = 0;
};

struct AudioGraphManager_RTE : AudioGraphManager
{
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::map<std::string, AudioGraphFile*> files;
	
	AudioGraphFile * selectedFile;
	
	SDL_mutex * audioMutex;
	
	AudioValueHistorySet * audioValueHistorySet;
	
	AudioGraphGlobals * globals;
	
	int displaySx;
	int displaySy;
	
	AudioGraphManager_RTE(const int displaySx, const int displaySy);
	virtual ~AudioGraphManager_RTE() override;
	
	// called from the app thread
	void init(SDL_mutex * mutex);
	void shut();
	
	// called from the app thread
	void selectFile(const char * filename);
	void selectInstance(const AudioGraphInstance * instance);
	
	// called from the app thread
	virtual AudioGraphInstance * createInstance(const char * filename) override;
	virtual void free(AudioGraphInstance *& instance) override;
	
	// called from the audio thread
	virtual void tick(const float dt) override;
	virtual void tickVisualizers() override;
	
	// called from the app thread
	bool tickEditor(const float dt, const bool isInputCaptured);
	void drawEditor();
};

extern AudioGraphManager * g_audioGraphMgr;
