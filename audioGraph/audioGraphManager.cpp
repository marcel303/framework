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

#include "audioGraph.h"
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "audioTypeDB.h"
#include "graph.h"
#include "graphEdit.h"
#include "Log.h"
#include <algorithm>
#include <SDL2/SDL.h>

//

struct AudioGraphFileRTC : GraphEdit_RealTimeConnection
{
	AudioGraphFile * file;
	
	AudioGraphFileRTC()
		: GraphEdit_RealTimeConnection()
		, file(nullptr)
	{
	}
	
	virtual void loadBegin() override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->loadBegin();
	}

	virtual void loadEnd(GraphEdit & graphEdit) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->loadEnd(graphEdit);
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->nodeAdd(nodeId, typeName);
	}

	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->nodeRemove(nodeId);
	}

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->setNodeIsPassthrough(nodeId, isPassthrough);
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->setSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->setDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}

	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->clearSrcSocketValue(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getSrcSocketChannelData(nodeId, srcSocketIndex, srcSocketName, channels);
	}

	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getDstSocketChannelData(nodeId, dstSocketIndex, dstSocketName, channels);
	}
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		if (file->activeInstance != nullptr)
			file->activeInstance->realTimeConnection->handleSrcSocketPressed(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeDescription(nodeId, lines);
	}
	
	virtual int getNodeActivity(const GraphNodeId nodeId) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeActivity(nodeId);
	}

	virtual int getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getLinkActivity(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual int getNodeCpuHeatMax() const override
	{
		if (file->activeInstance == nullptr)
			return 0;
		else
			return file->activeInstance->realTimeConnection->getNodeCpuHeatMax();
	}

	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const override
	{
		if (file->activeInstance == nullptr)
			return 0;
		else
			return file->activeInstance->realTimeConnection->getNodeCpuTimeUs(nodeId);
	}
};

//

AudioGraphInstance::AudioGraphInstance()
	: audioGraph(nullptr)
	, realTimeConnection(nullptr)
{
}

AudioGraphInstance::~AudioGraphInstance()
{
	delete audioGraph;
	audioGraph = nullptr;
	
	if (realTimeConnection != nullptr)
	{
		realTimeConnection->audioGraph = nullptr;
		realTimeConnection->audioGraphPtr = nullptr;
	}
	
	delete realTimeConnection;
	realTimeConnection = nullptr;
}

//

AudioGraphFile::AudioGraphFile()
	: filename()
	, instanceList()
	, activeInstance(nullptr)
	, realTimeConnection(nullptr)
	, audioValueHistorySet(nullptr)
	, graphEdit(nullptr)
{
	realTimeConnection = new AudioGraphFileRTC();
	realTimeConnection->file = this;
	
	audioValueHistorySet = new AudioValueHistorySet();
}

AudioGraphFile::~AudioGraphFile()
{
	Assert(instanceList.empty());
	
	for (auto & instance : instanceList)
	{
		delete instance;
		instance = nullptr;
	}
	
	instanceList.clear();
	
	//
	
	Assert(activeInstance == nullptr);
	activeInstance = nullptr;
	
	delete audioValueHistorySet;
	audioValueHistorySet = nullptr;
	
	delete realTimeConnection;
	realTimeConnection = nullptr;
	
	delete graphEdit;
	graphEdit = nullptr;
}

//

#include "graph.h"

AudioGraphManager_Basic::AudioGraphManager_Basic(bool _cacheOnCreate)
	: AudioGraphManager()
	, typeDefinitionLibrary(nullptr)
	, graphCache()
	, cacheOnCreate(false)
	, instances()
	, audioMutex(nullptr)
	, allocatedGlobals()
	, globals(nullptr)
{
	cacheOnCreate = _cacheOnCreate;
}

AudioGraphManager_Basic::~AudioGraphManager_Basic()
{
	shut();
}

void AudioGraphManager_Basic::init(SDL_mutex * mutex, AudioVoiceManager * voiceMgr)
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex.mutex = mutex;
	
	globals = createGlobals(mutex, voiceMgr);
}

void AudioGraphManager_Basic::shut()
{
	while (!instances.empty())
	{
		AudioGraphInstance *& instance = instances.front();
		
		free(instance, false);
	}
	
	//
	
	if (globals != nullptr)
	{
		globals->shut();
		
		freeGlobals(globals);
	}
	
	audioMutex.mutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
	
	for (auto & i : graphCache)
		delete i.second.graph;
	graphCache.clear();
}

void AudioGraphManager_Basic::addGraphToCache(const char * filename)
{
	auto i = graphCache.find(filename);
	
	if (i == graphCache.end())
	{
		auto e = graphCache.emplace(filename, GraphCacheElem());
		
		auto & elem = e.first->second;
		
		elem.graph = new Graph();
		elem.isValid = elem.graph->load(filename, typeDefinitionLibrary);
	}
}

AudioGraphGlobals * AudioGraphManager_Basic::createGlobals(SDL_mutex * mutex, AudioVoiceManager * voiceMgr)
{
	AudioGraphGlobals * globals = new AudioGraphGlobals();
	
	globals->init(mutex, voiceMgr, this);
	
	audioMutex.lock();
	{
		allocatedGlobals.insert(globals);
	}
	audioMutex.unlock();
	
	return globals;
}

void AudioGraphManager_Basic::freeGlobals(AudioGraphGlobals *& globals)
{
	// prune ramp down instances referencing these globals
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & instance : instances)
	{
		if (instance->audioGraph->rampDown && instance->audioGraph->globals == globals)
		{
			instancesToRemove.push_back(instance);
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// actually remove the globals
	
	audioMutex.lock();
	{
		allocatedGlobals.erase(globals);
	}
	audioMutex.unlock();
	
	delete globals;
	globals = nullptr;
}

AudioGraphInstance * AudioGraphManager_Basic::createInstance(const char * filename, AudioGraphGlobals * globals, const bool createdPaused)
{
	if (globals == nullptr)
	{
		globals = this->globals;
	}
	
	AudioGraph * audioGraph = nullptr;
	
	auto graphItr = graphCache.find(filename);
	
	if (graphItr != graphCache.end())
	{
		auto & elem = graphItr->second;
		
		if (elem.isValid)
		{
			audioGraph = constructAudioGraph(*elem.graph, typeDefinitionLibrary, globals, createdPaused);
		}
	}
	else
	{
		if (cacheOnCreate)
		{
			auto e = graphCache.emplace(filename, GraphCacheElem());
			
			auto & elem = e.first->second;
			
			elem.graph = new Graph();
			elem.isValid = elem.graph->load(filename, typeDefinitionLibrary);
			
			if (elem.isValid)
			{
				audioGraph = constructAudioGraph(*elem.graph, typeDefinitionLibrary, globals, createdPaused);
			}
		}
		else
		{
			Graph graph;
			
			if (graph.load(filename, typeDefinitionLibrary))
			{
				audioGraph = constructAudioGraph(graph, typeDefinitionLibrary, globals, createdPaused);
			}
		}
	}
	
	if (audioGraph != nullptr)
	{
		AudioGraphInstance * instance = new AudioGraphInstance();
		instance->audioGraph = audioGraph;
		
		audioMutex.lock();
		{
			instances.push_back(instance);
		}
		audioMutex.unlock();
		
		return instance;
	}
	else
	{
		return nullptr;
	}
}

void AudioGraphManager_Basic::free(AudioGraphInstance *& instance, const bool doRampDown)
{
	if (instance == nullptr)
	{
		return;
	}
	
	if (doRampDown)
	{
		instance->audioGraph->rampDownRequested = true;
		
		instance = nullptr;
		
		return;
	}
	
	for (auto instanceItr = instances.begin(); instanceItr != instances.end(); ++instanceItr)
	{
		if (*instanceItr == instance)
		{
			audioMutex.lock();
			{
				instances.erase(instanceItr);
			}
			audioMutex.unlock();
			
			delete instance;
			instance = nullptr;
			
			break;
		}
	}
	
	Assert(instance == nullptr); // found and freed?
}

void AudioGraphManager_Basic::tickMain()
{
	// prune instances
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & instance : instances)
	{
		if (instance->audioGraph->rampedDown)
		{
			instancesToRemove.push_back(instance);
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// tick audio graphs
	
	for (auto & instance : instances)
	{
		instance->audioGraph->tickMain();
	}
}

void AudioGraphManager_Basic::tickAudio(const float dt)
{
	audioMutex.lock();
	{
		for (auto & globals : allocatedGlobals)
			globals->tick(dt);
		
		// synchronize state from main to audio thread
		
		for (auto & instance : instances)
			instance->audioGraph->syncMainToAudio();
		
		// tick graph instances
		
		for (auto & instance : instances)
			instance->audioGraph->tickAudio(dt, false);
	}
	audioMutex.unlock();
}

void AudioGraphManager_Basic::tickVisualizers()
{
}

//

AudioGraphManager_RTE::AudioGraphManager_RTE(const int _displaySx, const int _displaySy)
	: AudioGraphManager()
	, typeDefinitionLibrary(nullptr)
	, files()
	, selectedFile(nullptr)
	, audioMutex(nullptr)
	, allocatedGlobals()
	, globals(nullptr)
	, displaySx(0)
	, displaySy(0)
{
	displaySx = _displaySx;
	displaySy = _displaySy;
}

AudioGraphManager_RTE::~AudioGraphManager_RTE()
{
	shut();
}

void AudioGraphManager_RTE::init(SDL_mutex * mutex, AudioVoiceManager * voiceMgr)
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex = mutex;
	
	globals = createGlobals(mutex, voiceMgr);
}

void AudioGraphManager_RTE::shut()
{
	for (auto & file : files)
	{
		delete file.second;
		file.second = nullptr;
	}
	
	files.clear();
	
	selectedFile = nullptr;
	
	//
	
	if (globals != nullptr)
	{
		globals->shut();
		
		freeGlobals(globals);
	}
	
	audioMutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

void AudioGraphManager_RTE::selectFile(const char * filename)
{
	SDL_LockMutex(audioMutex);
	{
		AudioGraphFile * newSelectedFile = nullptr;
		
		if (filename != nullptr)
		{
			auto fileItr = files.find(filename);
			
			if (fileItr != files.end())
			{
				newSelectedFile = fileItr->second;
			}
		}
		
		if (newSelectedFile != selectedFile)
		{
			if (selectedFile != nullptr)
			{
				selectedFile->graphEdit->cancelEditing();
				
				selectedFile = nullptr;
			}
			
			//
			
			selectedFile = newSelectedFile;
			
			//
			
			if (selectedFile != nullptr)
			{
				selectedFile->graphEdit->beginEditing();
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
}

void AudioGraphManager_RTE::selectInstance(const AudioGraphInstance * instance)
{
	SDL_LockMutex(audioMutex);
	{
		if (instance == nullptr)
		{
			selectFile(nullptr);
		}
		else
		{
			for (auto & fileItr : files)
			{
				auto file = fileItr.second;
				
				for (auto & instanceInFile : file->instanceList)
				{
					if (instance == instanceInFile)
					{
						selectFile(fileItr.first.c_str());
						
						file->activeInstance = instance;
					}
				}
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
}

AudioGraphGlobals * AudioGraphManager_RTE::createGlobals(SDL_mutex * mutex, AudioVoiceManager * voiceMgr)
{
	AudioGraphGlobals * globals = new AudioGraphGlobals();
	
	globals->init(mutex, voiceMgr, this);
	
	SDL_LockMutex(audioMutex);
	{
		allocatedGlobals.insert(globals);
	}
	SDL_UnlockMutex(audioMutex);
	
	return globals;
}

void AudioGraphManager_RTE::freeGlobals(AudioGraphGlobals *& globals)
{
	// prune ramp down instances referencing these globals
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			if (instance->audioGraph->rampDown && instance->audioGraph->globals == globals)
			{
				instancesToRemove.push_back(instance);
			}
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// actually remove the globals
	
	SDL_LockMutex(audioMutex);
	{
		allocatedGlobals.erase(globals);
	}
	SDL_UnlockMutex(audioMutex);
	
	delete globals;
	globals = nullptr;
}

AudioGraphInstance * AudioGraphManager_RTE::createInstance(const char * filename, AudioGraphGlobals * globals, const bool createdPaused)
{
	if (globals == nullptr)
	{
		globals = this->globals;
	}
	
	AudioGraphFile * file;
	bool isNew;
	
	SDL_LockMutex(audioMutex);
	{
		auto fileItr = files.find(filename);
		
		if (fileItr == files.end())
		{
			file = nullptr;
			isNew = true;
		}
		else
		{
			file = fileItr->second;
			isNew = false;
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	if (isNew)
	{
		file = new AudioGraphFile();
		file->filename = filename;
		
		file->graphEdit = new GraphEdit(displaySx, displaySy, typeDefinitionLibrary);
		file->graphEdit->realTimeConnection = file->realTimeConnection;
		
		file->graphEdit->load(filename);
	}
	
	//
	
	auto audioGraph = constructAudioGraph(*file->graphEdit->graph, typeDefinitionLibrary, globals, createdPaused);
	auto realTimeConnection = new AudioRealTimeConnection(file->audioValueHistorySet, globals);
	
	//
	
	AudioGraphInstance * instance = new AudioGraphInstance();
	instance->audioGraph = audioGraph;
	instance->realTimeConnection = realTimeConnection;
	instance->realTimeConnection->audioMutex = audioMutex;
	instance->realTimeConnection->audioGraph = instance->audioGraph;
	instance->realTimeConnection->audioGraphPtr = &instance->audioGraph;
	
	SDL_LockMutex(audioMutex);
	{
		if (isNew)
		{
			files[filename] = file;
		}
		
		file->instanceList.push_back(instance);
		
		if (file->activeInstance == nullptr)
		{
			file->activeInstance = instance;
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	return instance;
}

void AudioGraphManager_RTE::free(AudioGraphInstance *& instance, const bool doRampDown)
{
	if (instance == nullptr)
	{
		return;
	}
	
	if (doRampDown)
	{
		instance->audioGraph->rampDownRequested = true;
		
		instance = nullptr;
		
		return;
	}
	
	bool found = false;
	
	SDL_LockMutex(audioMutex);
	{
		for (auto fileItr = files.begin(); fileItr != files.end(); )
		{
			auto & file = fileItr->second;
			
			for (auto instanceItr = file->instanceList.begin(); instanceItr != file->instanceList.end(); )
			{
				if (*instanceItr == instance)
				{
					found = true;
					
					instanceItr = file->instanceList.erase(instanceItr);
					
					if (instance == file->activeInstance)
					{
						if (file->instanceList.empty())
						{
							file->activeInstance = nullptr;
						}
						else
						{
							file->activeInstance = file->instanceList.front();
						}
					}
				}
				else
				{
					instanceItr++;
				}
			}
			
		#if 0
			if (file->instanceList.empty())
			{
				if (file == selectedFile)
				{
					selectedFile = nullptr;
				}
				
				delete file;
				file = nullptr;
				
				fileItr = files.erase(fileItr);
			}
			else
		#endif
			{
				fileItr++;
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	Assert(found);
	
	delete instance;
	instance = nullptr;
}

void AudioGraphManager_RTE::tickMain()
{
	// prune instances
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			if (instance->audioGraph->rampedDown)
			{
				instancesToRemove.push_back(instance);
			}
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// tick audio graphs
	
	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			instance->audioGraph->tickMain();
		}
	}
}

void AudioGraphManager_RTE::tickAudio(const float dt)
{
	SDL_LockMutex(audioMutex);
	{
		for (auto & globals : allocatedGlobals)
			globals->tick(dt);
		
		// synchronize state from main to audio thread
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance->audioGraph->syncMainToAudio();
		
		// tick graph instances
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance->audioGraph->tickAudio(dt, false);
	}
	SDL_UnlockMutex(audioMutex);
}

void AudioGraphManager_RTE::tickVisualizers()
{
	SDL_LockMutex(audioMutex);
	{
		if (selectedFile && selectedFile->activeInstance)
		{
			selectedFile->activeInstance->realTimeConnection->updateAudioValues();
		}
	}
	SDL_UnlockMutex(audioMutex);
}

bool AudioGraphManager_RTE::tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured)
{
	bool result = false;
	
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->displaySx = sx;
		selectedFile->graphEdit->displaySy = sy;

		result |= selectedFile->graphEdit->tick(dt, isInputCaptured);
		
		selectedFile->graphEdit->tickVisualizers(dt);
	}
	
	return result;
}

void AudioGraphManager_RTE::drawEditor(const int sx, const int sy)
{
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->displaySx = sx;
		selectedFile->graphEdit->displaySy = sy;

		selectedFile->graphEdit->draw();
		
	#if 1 // todo : add a nice UI for drawing the filter response
		/*
		todo : let audio node define:
		- a custom editor interface
		- drop file behavior
		- custom draw
		- todo : remove getFilterResponse from audio nodes. it's too general. do add shared functions for analysing and drawing the response curve though!
		*/
		if (selectedFile->activeInstance != nullptr && selectedFile->graphEdit->selectedNodes.size() == 1)
		{
			auto audioGraph = selectedFile->activeInstance->audioGraph;
			
			const GraphNodeId nodeId = *selectedFile->graphEdit->selectedNodes.begin();
			
			auto nodeItr = audioGraph->nodes.find(nodeId);
	
			Assert(nodeItr != audioGraph->nodes.end());
			if (nodeItr != audioGraph->nodes.end())
			{
				const AudioNodeBase * audioNode = nodeItr->second;
				
				drawFilterResponse(audioNode, 200.f, 100.f);
			}
		}
	#endif
	}
}

//

AudioGraphManager_MultiRTE::AudioGraphManager_MultiRTE(const int _displaySx, const int _displaySy)
	: AudioGraphManager()
	, typeDefinitionLibrary(nullptr)
	, files()
	, selectedFile(nullptr)
	, audioMutex(nullptr)
	, allocatedGlobals()
	, globals(nullptr)
	, displaySx(0)
	, displaySy(0)
{
	displaySx = _displaySx;
	displaySy = _displaySy;
}

AudioGraphManager_MultiRTE::~AudioGraphManager_MultiRTE()
{
	shut();
}

void AudioGraphManager_MultiRTE::init(SDL_mutex * mutex, AudioVoiceManager * voiceMgr)
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex = mutex;
	
	globals = createGlobals(mutex, voiceMgr);
}

void AudioGraphManager_MultiRTE::shut()
{
	for (auto & file : files)
	{
		delete file.second;
		file.second = nullptr;
	}
	
	files.clear();
	
	selectedFile = nullptr;
	
	//
	
	if (globals != nullptr)
	{
		globals->shut();
		
		freeGlobals(globals);
	}
	
	audioMutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

void AudioGraphManager_MultiRTE::selectFile(const char * filename)
{
	SDL_LockMutex(audioMutex);
	{
		selectedFile = nullptr;
		
		if (filename != nullptr)
		{
			auto fileItr = files.find(filename);
			
			if (fileItr != files.end())
			{
				selectedFile = fileItr->second;
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
}

void AudioGraphManager_MultiRTE::selectInstance(const AudioGraphInstance * instance)
{
	SDL_LockMutex(audioMutex);
	{
		if (instance == nullptr)
		{
			selectFile(nullptr);
		}
		else
		{
			for (auto & fileItr : files)
			{
				auto file = fileItr.second;
				
				for (auto & instanceInFile : file->instanceList)
				{
					if (instance == instanceInFile)
					{
						selectFile(fileItr.first.c_str());
						
						file->activeInstance = instance;
					}
				}
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
}

AudioGraphGlobals * AudioGraphManager_MultiRTE::createGlobals(SDL_mutex * mutex, AudioVoiceManager * voiceMgr)
{
	AudioGraphGlobals * globals = new AudioGraphGlobals();
	
	globals->init(mutex, voiceMgr, this);
	
	SDL_LockMutex(audioMutex);
	{
		allocatedGlobals.insert(globals);
	}
	SDL_UnlockMutex(audioMutex);
	
	return globals;
}

void AudioGraphManager_MultiRTE::freeGlobals(AudioGraphGlobals *& globals)
{
	// prune ramp down instances referencing these globals
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			if (instance->audioGraph->rampDown && instance->audioGraph->globals == globals)
			{
				instancesToRemove.push_back(instance);
			}
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// actually remove the globals
	
	SDL_LockMutex(audioMutex);
	{
		allocatedGlobals.erase(globals);
	}
	SDL_UnlockMutex(audioMutex);
	
	delete globals;
	globals = nullptr;
}

AudioGraphInstance * AudioGraphManager_MultiRTE::createInstance(const char * filename, AudioGraphGlobals * globals, const bool createdPaused)
{
	if (globals == nullptr)
	{
		globals = this->globals;
	}
	
	AudioGraphFile * file;
	bool isNew;
	
	SDL_LockMutex(audioMutex);
	{
		auto fileItr = files.find(filename);
		
		if (fileItr == files.end())
		{
			file = nullptr;
			isNew = true;
		}
		else
		{
			file = fileItr->second;
			isNew = false;
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	if (isNew)
	{
		file = new AudioGraphFile();
		file->filename = filename;
		
		file->graphEdit = new GraphEdit(displaySx, displaySy, typeDefinitionLibrary);
		file->graphEdit->realTimeConnection = file->realTimeConnection;
		
		file->graphEdit->load(filename);
	}
	
	//
	
	auto audioGraph = constructAudioGraph(*file->graphEdit->graph, typeDefinitionLibrary, globals, createdPaused);
	auto realTimeConnection = new AudioRealTimeConnection(file->audioValueHistorySet, globals);
	
	//
	
	AudioGraphInstance * instance = new AudioGraphInstance();
	instance->audioGraph = audioGraph;
	instance->realTimeConnection = realTimeConnection;
	instance->realTimeConnection->audioMutex = audioMutex;
	instance->realTimeConnection->audioGraph = instance->audioGraph;
	instance->realTimeConnection->audioGraphPtr = &instance->audioGraph;
	
	SDL_LockMutex(audioMutex);
	{
		if (isNew)
		{
			files[filename] = file;
		}
		
		file->instanceList.push_back(instance);
		
		if (file->activeInstance == nullptr)
		{
			file->activeInstance = instance;
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	return instance;
}

void AudioGraphManager_MultiRTE::free(AudioGraphInstance *& instance, const bool doRampDown)
{
	if (instance == nullptr)
	{
		return;
	}
	
	if (doRampDown)
	{
		instance->audioGraph->rampDownRequested = true;
		
		instance = nullptr;
		
		return;
	}
	
	bool found = false;
	
	SDL_LockMutex(audioMutex);
	{
		for (auto fileItr = files.begin(); fileItr != files.end(); )
		{
			auto & file = fileItr->second;
			
			for (auto instanceItr = file->instanceList.begin(); instanceItr != file->instanceList.end(); )
			{
				if (*instanceItr == instance)
				{
					found = true;
					
					instanceItr = file->instanceList.erase(instanceItr);
					
					if (instance == file->activeInstance)
					{
						if (file->instanceList.empty())
						{
							file->activeInstance = nullptr;
						}
						else
						{
							file->activeInstance = file->instanceList.front();
						}
					}
				}
				else
				{
					instanceItr++;
				}
			}
			
		#if 0
			if (file->instanceList.empty())
			{
				if (file == selectedFile)
				{
					selectedFile = nullptr;
				}
				
				delete file;
				file = nullptr;
				
				fileItr = files.erase(fileItr);
			}
			else
		#endif
			{
				fileItr++;
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	Assert(found);
	
	delete instance;
	instance = nullptr;
}

void AudioGraphManager_MultiRTE::tickMain()
{
	// prune instances
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			if (instance->audioGraph->rampedDown)
			{
				instancesToRemove.push_back(instance);
			}
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// tick audio graphs
	
	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			instance->audioGraph->tickMain();
		}
	}
}

void AudioGraphManager_MultiRTE::tickAudio(const float dt)
{
	SDL_LockMutex(audioMutex);
	{
		for (auto & globals : allocatedGlobals)
			globals->tick(dt);
		
		// synchronize state from main to audio thread
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance->audioGraph->syncMainToAudio();
		
		// tick graph instances
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance->audioGraph->tickAudio(dt, false);
	}
	SDL_UnlockMutex(audioMutex);
}

void AudioGraphManager_MultiRTE::tickVisualizers()
{
	SDL_LockMutex(audioMutex);
	{
		for (auto & fileItr : files)
		{
			AudioGraphFile * file = fileItr.second;
			
			if (file && file->activeInstance)
			{
				file->activeInstance->realTimeConnection->updateAudioValues();
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
}

bool AudioGraphManager_MultiRTE::tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured)
{
	bool result = false;
	
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->displaySx = sx;
		selectedFile->graphEdit->displaySy = sy;

		result |= selectedFile->graphEdit->tick(dt, isInputCaptured);
		
		selectedFile->graphEdit->tickVisualizers(dt);
	}
	
	return result;
}

void AudioGraphManager_MultiRTE::drawEditor(const int sx, const int sy)
{
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->displaySx = sx;
		selectedFile->graphEdit->displaySy = sy;

		selectedFile->graphEdit->draw();
	}
}
