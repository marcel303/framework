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
#include "audioGraphManager.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "graph.h"
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
			instance.realTimeConnection->loadBegin();
	}

	virtual void loadEnd(GraphEdit & graphEdit) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->loadEnd(graphEdit);
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->nodeAdd(nodeId, typeName);
	}

	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->nodeRemove(nodeId);
	}

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->setNodeIsPassthrough(nodeId, isPassthrough);
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->setSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
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
			instance.realTimeConnection->setDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
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
			instance.realTimeConnection->clearSrcSocketValue(nodeId, srcSocketIndex, srcSocketName);
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
	
	virtual int nodeIsActive(const GraphNodeId nodeId) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->nodeIsActive(nodeId);
	}

	virtual int linkIsActive(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->linkIsActive(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual int getNodeCpuHeatMax() const override
	{
		if (file->activeInstance == nullptr)
			return 1000 * 1000;
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
	, graphEdit(nullptr)
{
	realTimeConnection = new AudioGraphFileRTC();
	realTimeConnection->file = this;
}

AudioGraphFile::~AudioGraphFile()
{
	Assert(instanceList.empty());
	instanceList.clear();
	
	Assert(activeInstance == nullptr);
	activeInstance = nullptr;
	
	delete realTimeConnection;
	realTimeConnection = nullptr;
	
	delete graphEdit;
	graphEdit = nullptr;
}

//

AudioGraphGlobals::AudioGraphGlobals()
	: controlValues()
	, memf()
	, audioMutex(nullptr)
	, audioGraphMgr(nullptr)
{
}

void AudioGraphGlobals::init(SDL_mutex * mutex, AudioVoiceManager * _voiceMgr, AudioGraphManager * _audioGraphMgr)
{
	audioMutex = mutex;
	
	voiceMgr = _voiceMgr;
	
	audioGraphMgr = _audioGraphMgr;
}

void AudioGraphGlobals::shut()
{
	audioGraphMgr = nullptr;
	
	audioMutex = nullptr;
}

void AudioGraphGlobals::tick(const float dt)
{
	// update control values

	for (auto & controlValue : controlValues)
	{
		const float retain = std::powf(controlValue.smoothness, dt);
		
		controlValue.currentX = controlValue.currentX * retain + controlValue.desiredX * (1.f - retain);
		controlValue.currentY = controlValue.currentY * retain + controlValue.desiredY * (1.f - retain);
	}

	exportControlValues();
}

void AudioGraphGlobals::registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY)
{
	SDL_LockMutex(audioMutex);
	{
		bool exists = false;
		
		for (auto & controlValue : controlValues)
		{
			if (controlValue.name == name)
			{
				controlValue.refCount++;
				exists = true;
				break;
			}
		}
		
		if (exists == false)
		{
			controlValues.resize(controlValues.size() + 1);
			
			auto & controlValue = controlValues.back();
			
			controlValue.type = type;
			controlValue.name = name;
			controlValue.refCount = 1;
			controlValue.min = min;
			controlValue.max = max;
			controlValue.smoothness = smoothness;
			controlValue.defaultX = defaultX;
			controlValue.defaultY = defaultY;
			controlValue.desiredX = defaultX;
			controlValue.desiredY = defaultY;
			controlValue.currentX = defaultX;
			controlValue.currentY = defaultY;
			
			std::sort(controlValues.begin(), controlValues.end(), [](const AudioControlValue & a, const AudioControlValue & b) { return a.name < b.name; });
		}
	}
	SDL_UnlockMutex(audioMutex);
}

void AudioGraphGlobals::unregisterControlValue(const char * name)
{
	SDL_LockMutex(audioMutex);
	{
		bool exists = false;
		
		for (auto controlValueItr = controlValues.begin(); controlValueItr != controlValues.end(); ++controlValueItr)
		{
			auto & controlValue = *controlValueItr;
			
			if (controlValue.name == name)
			{
				controlValue.refCount--;
				
				if (controlValue.refCount == 0)
				{
					//LOG_DBG("erasing control value %s", name);
					
					controlValues.erase(controlValueItr);
				}
				
				exists = true;
				break;
			}
		}
		
		Assert(exists);
		if (exists == false)
		{
			LOG_WRN("failed to unregister control value %s", name);
		}
	}
	SDL_UnlockMutex(audioMutex);
}

bool AudioGraphGlobals::findControlValue(const char * name, AudioControlValue & result) const
{
	bool found = false;
	
	SDL_LockMutex(audioMutex);
	{
		for (auto controlValueItr = controlValues.begin(); controlValueItr != controlValues.end(); ++controlValueItr)
		{
			auto & controlValue = *controlValueItr;
			
			if (controlValue.name == name)
			{
				result = controlValue;
				found = true;
				break;
			}
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	return found;
}

void AudioGraphGlobals::exportControlValues()
{
	SDL_LockMutex(audioMutex);
	{
		for (auto & controlValue : controlValues)
		{
			setMemf(controlValue.name.c_str(), controlValue.currentX, controlValue.currentY);
		}
	}
	SDL_UnlockMutex(audioMutex);
}

void AudioGraphGlobals::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	SDL_LockMutex(audioMutex);
	{
		auto & mem = memf[name];
		
		mem.value1 = value1;
		mem.value2 = value2;
		mem.value3 = value3;
		mem.value4 = value4;
	}
	SDL_UnlockMutex(audioMutex);
}

AudioGraphGlobals::Memf AudioGraphGlobals::getMemf(const char * name)
{
	Memf result;
	
	SDL_LockMutex(audioMutex);
	{
		auto memfItr = memf.find(name);
		
		if (memfItr != memf.end())
		{
			result = memfItr->second;
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	return result;
}

//

AudioGraphManager_Basic::AudioGraphManager_Basic(bool _cacheOnCreate)
	: AudioGraphManager()
	, typeDefinitionLibrary(nullptr)
	, graphCache()
	, cacheOnCreate(false)
	, instances()
	, audioMutex(nullptr)
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
	
	typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex.mutex = mutex;
	
	globals = new AudioGraphGlobals();
	globals->init(mutex, voiceMgr, this);
}

void AudioGraphManager_Basic::shut()
{
	while (!instances.empty())
	{
		AudioGraphInstance * instance = &instances.front();
		
		free(instance);
	}
	
	//
	
	if (globals != nullptr)
	{
		globals->shut();
		
		delete globals;
		globals = nullptr;
	}
	
	audioMutex.mutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

void AudioGraphManager_Basic::addGraphToCache(const char * filename)
{
	auto i = graphCache.find(filename);
	
	if (i == graphCache.end())
	{
		auto e = graphCache.emplace(filename, GraphCacheElem());
		
		auto & elem = e.first->second;
		
		elem.isValid = elem.graph.load(filename, typeDefinitionLibrary);
	}
}

AudioGraphInstance * AudioGraphManager_Basic::createInstance(const char * filename)
{
	AudioGraph * audioGraph = nullptr;
	
	auto graphItr = graphCache.find(filename);
	
	if (graphItr != graphCache.end())
	{
		auto & elem = graphItr->second;
		
		if (elem.isValid)
		{
			audioGraph = constructAudioGraph(elem.graph, typeDefinitionLibrary, globals);
		}
	}
	else
	{
		if (cacheOnCreate)
		{
			auto e = graphCache.emplace(filename, GraphCacheElem());
			
			auto & elem = e.first->second;
			
			elem.isValid = elem.graph.load(filename, typeDefinitionLibrary);
			
			if (elem.isValid)
			{
				audioGraph = constructAudioGraph(elem.graph, typeDefinitionLibrary, globals);
			}
		}
		else
		{
			Graph graph;
			
			if (graph.load(filename, typeDefinitionLibrary))
			{
				audioGraph = constructAudioGraph(graph, typeDefinitionLibrary, globals);
			}
		}
	}
	
	if (audioGraph != nullptr)
	{
		AudioGraphInstance * instance = nullptr;
		
		audioMutex.lock();
		{
			instances.emplace_back();
			instance = &instances.back();
			instance->audioGraph = audioGraph;
		}
		audioMutex.unlock();
		
		return instance;
	}
	else
	{
		return nullptr;
	}
}

void AudioGraphManager_Basic::free(AudioGraphInstance *& instance)
{
	if (instance == nullptr)
	{
		return;
	}
	
	for (auto instanceItr = instances.begin(); instanceItr != instances.end(); ++instanceItr)
	{
		// todo : remove lock around free. for now needed to avoid voices being referenced by voice manager during mixing while the graph is being destroyed. introduce a separate step to unregister voices ? perhaps add a voice list to each audio graph instance ?
		if (&(*instanceItr) == instance)
		{
			audioMutex.lock();
			instances.erase(instanceItr);
			audioMutex.unlock();
			
			instance = nullptr;
			
			break;
		}
	}
	
	Assert(instance == nullptr); // found and freed?
}

void AudioGraphManager_Basic::tick(const float dt)
{
	audioMutex.lock();
	{
		globals->tick(dt);
		
		// tick graph instances
		
		for (auto & instance : instances)
			instance.audioGraph->tick(dt);
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
	, audioValueHistorySet(nullptr)
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
	
	typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex = mutex;
	
	audioValueHistorySet = new AudioValueHistorySet();
	
	globals = new AudioGraphGlobals();
	globals->init(mutex, voiceMgr, this);
}

void AudioGraphManager_RTE::shut()
{
	for (auto & file : files)
		delete file.second;
	files.clear();
	
	selectedFile = nullptr;
	
	//
	
	if (globals != nullptr)
	{
		globals->shut();
		
		delete globals;
		globals = nullptr;
	}
	
	delete audioValueHistorySet;
	audioValueHistorySet = nullptr;
	
	audioMutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

void AudioGraphManager_RTE::selectFile(const char * filename)
{
	SDL_LockMutex(audioMutex);
	{
		if (selectedFile != nullptr)
		{
			selectedFile->graphEdit->cancelEditing();
			
			selectedFile = nullptr;
		}
		
		//
		
		if (filename != nullptr)
		{
			auto fileItr = files.find(filename);
			
			if (fileItr != files.end())
			{
				selectedFile = fileItr->second;
				
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
					if (instance == &instanceInFile)
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

AudioGraphInstance * AudioGraphManager_RTE::createInstance(const char * filename)
{
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
	
	auto audioGraph = constructAudioGraph(*file->graphEdit->graph, typeDefinitionLibrary, globals);
	auto realTimeConnection = new AudioRealTimeConnection(audioValueHistorySet, globals);
	
	//
	
	AudioGraphInstance * instance;
	
	SDL_LockMutex(audioMutex);
	{
		if (isNew)
		{
			files[filename] = file;
		}
		
		file->instanceList.push_back(AudioGraphInstance());
		
		instance = &file->instanceList.back();
		
		instance->audioGraph = audioGraph;
		instance->realTimeConnection = realTimeConnection;
		instance->realTimeConnection->audioMutex = audioMutex;
		instance->realTimeConnection->audioGraph = instance->audioGraph;
		instance->realTimeConnection->audioGraphPtr = &instance->audioGraph;
		
		if (file->activeInstance == nullptr)
		{
			file->activeInstance = instance;
		}
	}
	SDL_UnlockMutex(audioMutex);
	
	return instance;
}

void AudioGraphManager_RTE::free(AudioGraphInstance *& instance)
{
	if (instance == nullptr)
	{
		return;
	}
	
	// todo : move audio graph free outside of mutex scope
	
	SDL_LockMutex(audioMutex);
	{
		for (auto fileItr = files.begin(); fileItr != files.end(); )
		{
			auto & file = fileItr->second;
			
			for (auto instanceItr = file->instanceList.begin(); instanceItr != file->instanceList.end(); )
			{
				if (&(*instanceItr) == instance)
				{
					const bool isActiveInstance = instance == file->activeInstance;
					
					instanceItr = file->instanceList.erase(instanceItr);
					instance = nullptr;
					
					if (isActiveInstance)
					{
						if (file->instanceList.empty())
						{
							file->activeInstance = nullptr;
						}
						else
						{
							file->activeInstance = &file->instanceList.front();
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
}

void AudioGraphManager_RTE::tick(const float dt)
{
	SDL_LockMutex(audioMutex);
	{
		globals->tick(dt);
		
		// tick graph instances
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance.audioGraph->tick(dt);
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

bool AudioGraphManager_RTE::tickEditor(const float dt, const bool isInputCaptured)
{
	bool result = false;
	
	if (selectedFile != nullptr)
	{
		result |= selectedFile->graphEdit->tick(dt, isInputCaptured);
		
		selectedFile->graphEdit->tickVisualizers(dt);
	}
	
	return result;
}

void AudioGraphManager_RTE::drawEditor()
{
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->draw();
	}
}
