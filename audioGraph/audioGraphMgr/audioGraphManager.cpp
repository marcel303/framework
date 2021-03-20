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

#include "audioGraph.h"
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "audioTypeDB.h"
#include "graph.h"
#include "graphEdit.h"
#include "Log.h"

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
	
	virtual void nodeAdd(const GraphNode & node) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->nodeAdd(node);
	}

	virtual void nodeInit(const GraphNode & node) override
	{
		for (auto & instance : file->instanceList)
			instance->realTimeConnection->nodeInit(node);
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
	, audioValueHistorySetCapture(nullptr)
	, graphEdit(nullptr)
{
	realTimeConnection = new AudioGraphFileRTC();
	realTimeConnection->file = this;
	
	audioValueHistorySet = new AudioValueHistorySet();
	audioValueHistorySetCapture = new AudioValueHistorySet();
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
	
	delete audioValueHistorySetCapture;
	audioValueHistorySetCapture = nullptr;
	
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
	, allocatedContexts()
	, context(nullptr)
{
	cacheOnCreate = _cacheOnCreate;
}

AudioGraphManager_Basic::~AudioGraphManager_Basic()
{
	shut();
}

void AudioGraphManager_Basic::init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr)
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex = mutex;
	
	mutex_mem.init();
	mutex_reg.init();
	
	context = createContext(voiceMgr);
}

void AudioGraphManager_Basic::shut()
{
	while (!instances.empty())
	{
		AudioGraphInstance *& instance = instances.front();
		
		free(instance, false);
	}
	
	//
	
	if (context != nullptr)
	{
		context->shut();
		
		freeContext(context);
	}
	
	mutex_mem.shut();
	mutex_reg.shut();
	
	audioMutex = nullptr;
	
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

AudioGraphContext * AudioGraphManager_Basic::createContext(AudioVoiceManager * voiceMgr)
{
	AudioGraphContext * context = new AudioGraphContext();
	
	context->init(&mutex_mem, &mutex_reg, voiceMgr);
	
	audioMutex->lock();
	{
		allocatedContexts.insert(context);
	}
	audioMutex->unlock();
	
	return context;
}

void AudioGraphManager_Basic::freeContext(AudioGraphContext *& context)
{
	// prune ramp down instances referencing this context
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & instance : instances)
	{
		if (instance->audioGraph->rampDown && instance->audioGraph->context == context)
		{
			instancesToRemove.push_back(instance);
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// actually remove the context
	
	audioMutex->lock();
	{
		allocatedContexts.erase(context);
	}
	audioMutex->unlock();
	
	delete context;
	context = nullptr;
}

AudioGraphContext * AudioGraphManager_Basic::getContext()
{
	return context;
}

AudioGraphInstance * AudioGraphManager_Basic::createInstance(const char * filename, AudioGraphContext * context, const bool createdPaused)
{
	if (context == nullptr)
	{
		context = this->context;
	}
	
	AudioGraph * audioGraph = nullptr;
	
	auto graphItr = graphCache.find(filename);
	
	if (graphItr != graphCache.end())
	{
		auto & elem = graphItr->second;
		
		if (elem.isValid)
		{
			audioGraph = constructAudioGraph(*elem.graph, typeDefinitionLibrary, context, createdPaused);
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
				audioGraph = constructAudioGraph(*elem.graph, typeDefinitionLibrary, context, createdPaused);
			}
		}
		else
		{
			Graph graph;
			
			if (graph.load(filename, typeDefinitionLibrary))
			{
				audioGraph = constructAudioGraph(graph, typeDefinitionLibrary, context, createdPaused);
			}
		}
	}
	
	if (audioGraph != nullptr)
	{
		AudioGraphInstance * instance = new AudioGraphInstance();
		instance->audioGraph = audioGraph;
		
		audioMutex->lock();
		{
			instances.push_back(instance);
		}
		audioMutex->unlock();
		
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
			audioMutex->lock();
			{
				instances.erase(instanceItr);
			}
			audioMutex->unlock();
			
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
}

void AudioGraphManager_Basic::tickAudio(const float dt)
{
	lockAudio();
	{
		mutex_mem.lock();
		{
			// synchronize state from main to audio thread
			
			for (auto & context : allocatedContexts)
				context->tickAudio(dt);
			
			for (auto & instance : instances)
				instance->audioGraph->syncMainToAudio(dt);
		}
		mutex_mem.unlock();
		
		// tick graph instances
		
		for (auto & instance : instances)
			instance->audioGraph->tickAudio(dt, false);
	}
	unlockAudio();
}

void AudioGraphManager_Basic::tickVisualizers()
{
}

void AudioGraphManager_Basic::lockAudio()
{
	audioMutex->lock();
}

void AudioGraphManager_Basic::unlockAudio()
{
	audioMutex->unlock();
}

//

AudioGraphManager_RTE::AudioGraphManager_RTE(const int in_displaySx, const int in_displaySy)
	: AudioGraphManager()
	, typeDefinitionLibrary(nullptr)
	, files()
	, selectedFile(nullptr)
	, audioMutex(nullptr)
	, mutex_mem()
	, mutex_reg()
	, allocatedContexts()
	, context(nullptr)
	, displaySx(0)
	, displaySy(0)
{
	displaySx = in_displaySx;
	displaySy = in_displaySy;
}

AudioGraphManager_RTE::~AudioGraphManager_RTE()
{
	shut();
}

void AudioGraphManager_RTE::init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr)
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex = mutex;
	
	mutex_mem.init();
	mutex_reg.init();
	
	context = createContext(voiceMgr);
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
	
	if (context != nullptr)
	{
		context->shut();
		
		freeContext(context);
	}
	
	mutex_mem.shut();
	mutex_reg.shut();
	
	audioMutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

void AudioGraphManager_RTE::selectFile(const char * filename)
{
	audioMutex->lock();
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
	audioMutex->unlock();
}

void AudioGraphManager_RTE::selectInstance(const AudioGraphInstance * instance)
{
	audioMutex->lock();
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
	audioMutex->unlock();
}

AudioGraphContext * AudioGraphManager_RTE::createContext(AudioVoiceManager * voiceMgr)
{
	AudioGraphContext * context = new AudioGraphContext();
	
	context->init(&mutex_mem, &mutex_reg, voiceMgr);
	
	audioMutex->lock();
	{
		allocatedContexts.insert(context);
	}
	audioMutex->unlock();
	
	return context;
}

void AudioGraphManager_RTE::freeContext(AudioGraphContext *& context)
{
	// prune ramp down instances referencing this context
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			if (instance->audioGraph->rampDown && instance->audioGraph->context == context)
			{
				instancesToRemove.push_back(instance);
			}
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// actually remove the context
	
	audioMutex->lock();
	{
		allocatedContexts.erase(context);
	}
	audioMutex->unlock();
	
	delete context;
	context = nullptr;
}

AudioGraphContext * AudioGraphManager_RTE::getContext()
{
	return context;
}

AudioGraphInstance * AudioGraphManager_RTE::createInstance(const char * filename, AudioGraphContext * context, const bool createdPaused)
{
	if (context == nullptr)
	{
		context = this->context;
	}
	
	AudioGraphFile * file;
	bool isNew;
	
	audioMutex->lock();
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
	audioMutex->unlock();
	
	if (isNew)
	{
		file = new AudioGraphFile();
		file->filename = filename;
		
		file->graphEdit = new GraphEdit(displaySx, displaySy, typeDefinitionLibrary);
		file->graphEdit->realTimeConnection = file->realTimeConnection;
		
		file->graphEdit->load(filename);
	}
	
	//
	
	AudioGraphInstance * instance = new AudioGraphInstance();
	
	auto * audioGraph = constructAudioGraph(*file->graphEdit->graph, typeDefinitionLibrary, context, createdPaused);
	instance->audioGraph = audioGraph;
	
	auto * realTimeConnection = new AudioRealTimeConnection(
		instance->audioGraph,
		context,
		file->audioValueHistorySet,
		file->audioValueHistorySetCapture);
	instance->realTimeConnection = realTimeConnection;
	instance->realTimeConnection->audioMutex = audioMutex;
	
	//
	
	audioMutex->lock();
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
	audioMutex->unlock();
	
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
	
	audioMutex->lock();
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
	audioMutex->unlock();
	
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
	
	// copy audio values
	
	if (selectedFile && selectedFile->activeInstance)
	{
		selectedFile->activeInstance->realTimeConnection->captureAudioValues();
	}
}

void AudioGraphManager_RTE::tickAudio(const float dt)
{
	lockAudio();
	{
		mutex_mem.lock();
		{
			// synchronize state from main to audio thread
			
			for (auto & context : allocatedContexts)
				context->tickAudio(dt);
			
			for (auto & file : files)
				for (auto & instance : file.second->instanceList)
					instance->audioGraph->syncMainToAudio(dt);
		}
		mutex_mem.unlock();
	
		// tick graph instances
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance->audioGraph->tickAudio(dt, false);
	}
	unlockAudio();
}

void AudioGraphManager_RTE::tickVisualizers()
{
	audioMutex->lock();
	{
		if (selectedFile && selectedFile->activeInstance)
		{
			selectedFile->activeInstance->realTimeConnection->updateAudioValues();
		}
	}
	audioMutex->unlock();
}

void AudioGraphManager_RTE::lockAudio()
{
	audioMutex->lock();
}

void AudioGraphManager_RTE::unlockAudio()
{
	audioMutex->unlock();
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
		
	#if 0 // todo : add a nice UI for drawing the filter response
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

AudioGraphManager_MultiRTE::AudioGraphManager_MultiRTE(const int in_displaySx, const int in_displaySy)
	: AudioGraphManager()
	, typeDefinitionLibrary(nullptr)
	, files()
	, selectedFile(nullptr)
	, audioMutex(nullptr)
	, allocatedContexts()
	, context(nullptr)
	, displaySx(0)
	, displaySy(0)
{
	displaySx = in_displaySx;
	displaySy = in_displaySy;
}

AudioGraphManager_MultiRTE::~AudioGraphManager_MultiRTE()
{
	shut();
}

void AudioGraphManager_MultiRTE::init(AudioMutexBase * mutex, AudioVoiceManager * voiceMgr)
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createAudioValueTypeDefinitions(*typeDefinitionLibrary);
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
	
	audioMutex = mutex;
	
	mutex_mem.init();
	mutex_reg.init();
	
	context = createContext(voiceMgr);
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
	
	if (context != nullptr)
	{
		context->shut();
		
		freeContext(context);
	}
	
	mutex_mem.shut();
	mutex_reg.shut();
	
	audioMutex = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

void AudioGraphManager_MultiRTE::selectFile(const char * filename)
{
	audioMutex->lock();
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
	audioMutex->unlock();
}

void AudioGraphManager_MultiRTE::selectInstance(const AudioGraphInstance * instance)
{
	audioMutex->lock();
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
	audioMutex->unlock();
}

AudioGraphContext * AudioGraphManager_MultiRTE::createContext(AudioVoiceManager * voiceMgr)
{
	AudioGraphContext * context = new AudioGraphContext();
	
	context->init(&mutex_mem, &mutex_reg, voiceMgr);
	
	audioMutex->lock();
	{
		allocatedContexts.insert(context);
	}
	audioMutex->unlock();
	
	return context;
}

void AudioGraphManager_MultiRTE::freeContext(AudioGraphContext *& context)
{
	// prune ramp down instances referencing this context
	
	std::vector<AudioGraphInstance*> instancesToRemove;

	for (auto & file : files)
	{
		for (auto & instance : file.second->instanceList)
		{
			if (instance->audioGraph->rampDown && instance->audioGraph->context == context)
			{
				instancesToRemove.push_back(instance);
			}
		}
	}
	
	for (auto & instanceToRemove : instancesToRemove)
	{
		free(instanceToRemove, false);
	}
	
	// actually remove the context
	
	audioMutex->lock();
	{
		allocatedContexts.erase(context);
	}
	audioMutex->unlock();
	
	delete context;
	context = nullptr;
}

AudioGraphContext * AudioGraphManager_MultiRTE::getContext()
{
	return context;
}

AudioGraphInstance * AudioGraphManager_MultiRTE::createInstance(const char * filename, AudioGraphContext * context, const bool createdPaused)
{
	if (context == nullptr)
	{
		context = this->context;
	}
	
	AudioGraphFile * file;
	bool isNew;
	
	audioMutex->lock();
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
	audioMutex->unlock();
	
	if (isNew)
	{
		file = new AudioGraphFile();
		file->filename = filename;
		
		file->graphEdit = new GraphEdit(displaySx, displaySy, typeDefinitionLibrary);
		file->graphEdit->realTimeConnection = file->realTimeConnection;
		
		file->graphEdit->load(filename);
	}
	
	//
	
	AudioGraphInstance * instance = new AudioGraphInstance();
	
	auto * audioGraph = constructAudioGraph(*file->graphEdit->graph, typeDefinitionLibrary, context, createdPaused);
	instance->audioGraph = audioGraph;
	
	auto * realTimeConnection = new AudioRealTimeConnection(
		instance->audioGraph,
		context,
		file->audioValueHistorySet,
		file->audioValueHistorySetCapture);
	instance->realTimeConnection = realTimeConnection;
	instance->realTimeConnection->audioMutex = audioMutex;
	
	audioMutex->lock();
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
	audioMutex->unlock();
	
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
	
	audioMutex->lock();
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
	audioMutex->unlock();
	
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
	
	// copy audio values
	
	audioMutex->lock(); // lock here, so we don't lock many times in captureAudioValues
	{
		for (auto & fileItr : files)
		{
			AudioGraphFile * file = fileItr.second;
			
			if (file && file->activeInstance)
			{
				file->activeInstance->realTimeConnection->captureAudioValues();
			}
		}
	}
	audioMutex->unlock();
}

void AudioGraphManager_MultiRTE::tickAudio(const float dt)
{
	lockAudio();
	{
		mutex_mem.lock();
		{
			// synchronize state from main to audio thread
			
			for (auto & context : allocatedContexts)
				context->tickAudio(dt);
			
			for (auto & file : files)
				for (auto & instance : file.second->instanceList)
					instance->audioGraph->syncMainToAudio(dt);
		}
		mutex_mem.unlock();
		
		// tick graph instances
		
		for (auto & file : files)
			for (auto & instance : file.second->instanceList)
				instance->audioGraph->tickAudio(dt, false);
	}
	unlockAudio();
}

void AudioGraphManager_MultiRTE::tickVisualizers()
{
	audioMutex->lock();
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
	audioMutex->unlock();
}

void AudioGraphManager_MultiRTE::lockAudio()
{
	audioMutex->lock();
}

void AudioGraphManager_MultiRTE::unlockAudio()
{
	audioMutex->unlock();
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
