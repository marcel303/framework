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
#include "audioNodeBase.h"
#include "framework.h"
#include "Parse.h"

#if AUDIO_GRAPH_ENABLE_TIMING
	#include "Timer.h"
#endif

//

AudioGraph * g_currentAudioGraph = nullptr;

double g_currentAudioTime = 0.0;

//

AudioGraph::AudioGraph()
	: nodes()
	, nextTickTraversalId(0)
	, graph(nullptr)
	, valuesToFree()
	, time(0.0)
	, activeFlags()
	, events()
	, mutex(nullptr)
{
	mutex = SDL_CreateMutex();
	Assert(mutex != nullptr);
}

AudioGraph::~AudioGraph()
{
	destroy();
	
	Assert(mutex != nullptr);
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

void AudioGraph::destroy()
{
	for (auto i : valuesToFree)
	{
		switch (i.type)
		{
		case ValueToFree::kType_Bool:
			delete (bool*)i.mem;
			break;
		case ValueToFree::kType_Int:
			delete (int*)i.mem;
			break;
		case ValueToFree::kType_Float:
			delete (float*)i.mem;
			break;
		case ValueToFree::kType_String:
			delete (std::string*)i.mem;
			break;
		case ValueToFree::kType_AudioValue:
			delete (AudioFloat*)i.mem;
			break;
		default:
			Assert(false);
			break;
		}
	}
	
	valuesToFree.clear();
	
	for (auto i : nodes)
	{
		AudioNodeBase * node = i.second;
		
	#if AUDIO_GRAPH_ENABLE_TIMING
		const uint64_t t1 = g_TimerRT.TimeUS_get();
	#endif
		
		delete node;
		node = nullptr;
		
	#if AUDIO_GRAPH_ENABLE_TIMING
		const uint64_t t2 = g_TimerRT.TimeUS_get();
		auto graphNode = graph->tryGetNode(i.first);
		const std::string typeName = graphNode ? graphNode->typeName : "n/a";
		logDebug("delete %s took %.2fms", typeName.c_str(), (t2 - t1) / 1000.0);
	#endif
	}
	
	nodes.clear();
	
	graph = nullptr;
}

void AudioGraph::connectToInputLiteral(AudioPlug & input, const std::string & inputValue)
{
	if (input.type == kAudioPlugType_Bool)
	{
		bool * value = new bool();
		
		*value = Parse::Bool(inputValue);
		
		input.connectTo(value, kAudioPlugType_Bool, true);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Bool, value));
	}
	else if (input.type == kAudioPlugType_Int)
	{
		int * value = new int();
		
		*value = Parse::Int32(inputValue);
		
		input.connectTo(value, kAudioPlugType_Int, true);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Int, value));
	}
	else if (input.type == kAudioPlugType_Float)
	{
		float * value = new float();
		
		*value = Parse::Float(inputValue);
		
		input.connectTo(value, kAudioPlugType_Float, true);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Float, value));
	}
	else if (input.type == kAudioPlugType_String)
	{
		std::string * value = new std::string();
		
		*value = inputValue;
		
		input.connectTo(value, kAudioPlugType_String, true);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_String, value));
	}
	else if (input.type == kAudioPlugType_FloatVec)
	{
		const float scalarValue = Parse::Float(inputValue);
		
		AudioFloat * value = new AudioFloat(scalarValue);
		
		input.connectTo(value, kAudioPlugType_FloatVec, true);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_AudioValue, value));
	}
	else
	{
		logWarning("cannot instantiate literal for non-supported type %d, value=%s", input.type, inputValue.c_str());
	}
}

void AudioGraph::tick(const float dt)
{
	audioCpuTimingBlock(AudioGraph_Tick);
	
	Assert(g_currentAudioGraph == nullptr);
	g_currentAudioGraph = this;
	
	// process nodes
	
	for (auto i : nodes)
	{
		AudioNodeBase * node = i.second;
		
		if (node->lastTickTraversalId != nextTickTraversalId)
		{
			node->traverseTick(nextTickTraversalId, dt);
		}
	}
	
	++nextTickTraversalId;
	
	//
	
	time += dt;
	
	events.clear();
	
	//
	
	g_currentAudioGraph = nullptr;
}

void AudioGraph::setFlag(const char * name, const bool value)
{
	SDL_LockMutex(mutex);
	{
		if (value)
			activeFlags.insert(name);
		else
			activeFlags.erase(name);
	}
	SDL_UnlockMutex(mutex);
}

void AudioGraph::resetFlag(const char * name)
{
	SDL_LockMutex(mutex);
	{
		activeFlags.erase(name);
	}
	SDL_UnlockMutex(mutex);
}

bool AudioGraph::isFLagSet(const char * name) const
{
	bool result = false;
	
	SDL_LockMutex(mutex);
	{
		result = activeFlags.count(name) != 0;
	}
	SDL_UnlockMutex(mutex);
	
	return result;
}

void AudioGraph::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	SDL_LockMutex(mutex);
	{
		auto & mem = memf[name];
		
		mem.value1 = value1;
		mem.value2 = value2;
		mem.value3 = value3;
		mem.value4 = value4;
	}
	SDL_UnlockMutex(mutex);
}

AudioGraph::Memf AudioGraph::getMemf(const char * name) const
{
	Memf result;
	
	SDL_LockMutex(mutex);
	{
		auto memItr = memf.find(name);
		
		if (memItr != memf.end())
		{
			result = memItr->second;
		}
	}
	SDL_UnlockMutex(mutex);
	
	return result;
}

void AudioGraph::setMems(const char * name, const char * value)
{
	SDL_LockMutex(mutex);
	{
		auto & mem = mems[name];
		
		mem.value = value;
	}
	SDL_UnlockMutex(mutex);
}

AudioGraph::Mems AudioGraph::getMems(const char * name) const
{
	Mems result;
	
	SDL_LockMutex(mutex);
	{
		auto memItr = mems.find(name);
		
		if (memItr != mems.end())
		{
			result = memItr->second;
		}
	}
	SDL_UnlockMutex(mutex);
	
	return result;
}

void AudioGraph::triggerEvent(const char * event)
{
	SDL_LockMutex(mutex);
	{
		events.push_back(event);
	}
	SDL_UnlockMutex(mutex);
}

//

AudioNodeBase * createAudioNode(const GraphNodeId nodeId, const std::string & typeName, AudioGraph * audioGraph)
{
	AudioNodeBase * audioNode = nullptr;
	
	for (AudioNodeTypeRegistration * r = g_audioNodeTypeRegistrationList; r != nullptr; r = r->next)
	{
		if (r->typeName == typeName)
		{
		#if AUDIO_GRAPH_ENABLE_TIMING
			const uint64_t t1 = g_TimerRT.TimeUS_get();
		#endif
		
			audioNode = r->create();
			
		#if AUDIO_GRAPH_ENABLE_TIMING
			const uint64_t t2 = g_TimerRT.TimeUS_get();
			logDebug("create %s took %.2fms", typeName.c_str(), (t2 - t1) / 1000.0);
		#endif
		
			break;
		}
	}
	
	return audioNode;
}

//

AudioGraph * constructAudioGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	AudioGraph * audioGraph = new AudioGraph();
	
	audioGraph->graph = const_cast<Graph*>(&graph);
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		AudioNodeBase * audioNode = createAudioNode(node.id, node.typeName, audioGraph);
		
		Assert(audioNode != nullptr);
		if (audioNode == nullptr)
		{
			logError("unable to create node");
		}
		else
		{
			audioNode->isPassthrough = node.isPassthrough;
			
			audioNode->initSelf(node);
			
			audioGraph->nodes[node.id] = audioNode;
		}
	}
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
		if (link.isEnabled == false)
		{
			continue;
		}
		
		auto srcNodeItr = audioGraph->nodes.find(link.srcNodeId);
		auto dstNodeItr = audioGraph->nodes.find(link.dstNodeId);
		
		Assert(srcNodeItr != audioGraph->nodes.end() && dstNodeItr != audioGraph->nodes.end());
		if (srcNodeItr == audioGraph->nodes.end() || dstNodeItr == audioGraph->nodes.end())
		{
			if (srcNodeItr == audioGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == audioGraph->nodes.end())
				logError("destination node doesn't exist");
		}
		else
		{
			auto srcNode = srcNodeItr->second;
			auto dstNode = dstNodeItr->second;
			
			auto input = srcNode->tryGetInput(link.srcNodeSocketIndex);
			auto output = dstNode->tryGetOutput(link.dstNodeSocketIndex);
			
			Assert(input != nullptr && output != nullptr);
			if (input == nullptr || output == nullptr)
			{
				if (input == nullptr)
					logError("input node socket doesn't exist. name=%s, index=%d", link.srcNodeSocketName.c_str(), link.srcNodeSocketIndex);
				if (output == nullptr)
					logError("output node socket doesn't exist. name=%s, index=%d", link.dstNodeSocketName.c_str(), link.dstNodeSocketIndex);
			}
			else
			{
				input->connectTo(*output);
				
				// note : this may add the same node multiple times to the list of predeps. note that this
				//        is ok as nodes will be traversed once through the travel id + it works nicely
				//        with the live connection as we can just remove the predep and still have one or
				//        references to the predep if the predep was referenced more than once
				srcNode->predeps.push_back(dstNode);
				
				// if this is a trigger, add a trigger target to dstNode
				if (output->type == kAudioPlugType_Trigger)
				{
					AudioNodeBase::TriggerTarget triggerTarget;
					triggerTarget.srcNode = srcNode;
					triggerTarget.srcSocketIndex = link.srcNodeSocketIndex;
					triggerTarget.dstSocketIndex = link.dstNodeSocketIndex;
					
					dstNode->triggerTargets.push_back(triggerTarget);
				}
			}
		}
	}
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		auto typeDefintion = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefintion == nullptr)
			continue;
		
		auto audioNodeItr = audioGraph->nodes.find(node.id);
		
		if (audioNodeItr == audioGraph->nodes.end())
			continue;
		
		AudioNodeBase * audioNode = audioNodeItr->second;
		
		auto & audioNodeInputs = audioNode->inputs;
		
		for (auto inputValueItr : node.editorInputValues)
		{
			const std::string & inputName = inputValueItr.first;
			const std::string & inputValue = inputValueItr.second;
			
			for (size_t i = 0; i < typeDefintion->inputSockets.size(); ++i)
			{
				if (typeDefintion->inputSockets[i].name == inputName)
				{
					if (i < audioNodeInputs.size())
					{
						if (audioNodeInputs[i].isConnected() == false)
						{
							audioGraph->connectToInputLiteral(audioNodeInputs[i], inputValue);
						}
					}
				}
			}
		}
	}
	
	for (auto audioNodeItr : audioGraph->nodes)
	{
		auto nodeId = audioNodeItr.first;
		auto nodeItr = graph.nodes.find(nodeId);
		auto & node = nodeItr->second;
		auto audioNode = audioNodeItr.second;
		
		audioNode->init(node);
	}
	
	return audioGraph;
}

//

#include "Path.h"
#include "soundmix.h"
#include "StringEx.h"
#include <map>

#include "Timer.h" // todo : add routines for capturing and logging timing data

static std::map<std::string, PcmData*> s_pcmDataCache;

void fillPcmDataCache(const char * path, const bool recurse, const bool stripPaths)
{
	logDebug("filling data cache with path: %s", path);
	
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
		
		if (pcmData->load(filenameLower.c_str(), 0) == false)
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
	
	printf("load PCM from %s took %.2fms\n", path, (t2 - t1) / 1000.0);
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

//

#include "binaural.h"
#include "binaural_cipic.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"

std::map<std::string, binaural::HRIRSampleSet*> s_hrirSampleSetCache;

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
