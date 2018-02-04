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
#include "framework.h" // listFiles
#include "Log.h"
#include "Parse.h"

#if AUDIO_GRAPH_ENABLE_TIMING
	#include "Timer.h"
#endif

//

AUDIO_THREAD_LOCAL AudioGraph * g_currentAudioGraph = nullptr;

double g_currentAudioTime = 0.0;

//

AudioGraph::AudioGraph(AudioGraphGlobals * _globals, const bool _isPaused)
	: isPaused(_isPaused)
	, rampDownRequested(false)
	, rampDown(false)
	, rampedDown(false)
	, nodes()
	, currentTickTraversalId(-1)
#if AUDIO_GRAPH_ENABLE_TIMING
	, graph(nullptr)
#endif
	, valuesToFree()
	, time(0.0)
	, activeFlags()
	, memf()
	, mems()
	, events()
	, triggeredEvents()
	, controlValues()
	, globals(nullptr)
	, mutex()
{
	mutex.init();
	
	globals = _globals;
}

AudioGraph::~AudioGraph()
{
	destroy();
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
	
	Assert(g_currentAudioGraph == nullptr);
	g_currentAudioGraph = this;
	
	for (auto & i : nodes)
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
		LOG_DBG("delete %s took %.2fms", typeName.c_str(), (t2 - t1) / 1000.0);
	#endif
	}
	
	nodes.clear();
	
	Assert(g_currentAudioGraph == this);
	g_currentAudioGraph = nullptr;
	
#if AUDIO_GRAPH_ENABLE_TIMING
	graph = nullptr;
#endif

	mutex.shut();
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
		LOG_WRN("cannot instantiate literal for non-supported type %d, value=%s", input.type, inputValue.c_str());
	}
}

void AudioGraph::tick(const float dt)
{
	audioCpuTimingBlock(AudioGraph_Tick);
	
	if (isPaused)
		return;
	
	Assert(g_currentAudioGraph == nullptr);
	g_currentAudioGraph = this;
	
	if (rampDownRequested)
		rampDown = true;
	
	mutex.lock();
	{
		// update control values
		
		for (auto & controlValue : controlValues)
		{
			const float retain = powf(controlValue.smoothness, dt);
			
			controlValue.currentX = controlValue.currentX * retain + controlValue.desiredX * (1.f - retain);
			controlValue.currentY = controlValue.currentY * retain + controlValue.desiredY * (1.f - retain);
		}

		exportControlValues();
		
		// update the set of 'active' events. all newly triggered events are processed next tick
		
		std::swap(events, triggeredEvents);
		triggeredEvents.clear();
	}
	mutex.unlock();
		
	// process nodes
	
	++currentTickTraversalId;
	
	for (auto & i : nodes)
	{
		AudioNodeBase * node = i.second;
		
		if (node->lastTickTraversalId != currentTickTraversalId)
		{
			node->traverseTick(currentTickTraversalId, dt);
		}
	}
	
	//
	
	time += dt;
	
	//
	
	if (rampDown)
		rampedDown = true;
	
	//
	
	g_currentAudioGraph = nullptr;
}

void AudioGraph::setFlag(const char * name, const bool value)
{
	mutex.lock();
	{
		if (value)
			activeFlags.insert(name);
		else
			activeFlags.erase(name);
	}
	mutex.unlock();
}

void AudioGraph::resetFlag(const char * name)
{
	mutex.lock();
	{
		activeFlags.erase(name);
	}
	mutex.unlock();
}

bool AudioGraph::isFLagSet(const char * name) const
{
	bool result = false;
	
	mutex.lock();
	{
		result = activeFlags.count(name) != 0;
	}
	mutex.unlock();
	
	return result;
}

void AudioGraph::registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY)
{
	mutex.lock();
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
	mutex.unlock();
}

void AudioGraph::unregisterControlValue(const char * name)
{
	mutex.lock();
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
	mutex.unlock();
}

bool AudioGraph::findControlValue(const char * name, AudioControlValue & result) const
{
	bool found = false;
	
	mutex.lock();
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
	mutex.unlock();
	
	return found;
}

void AudioGraph::exportControlValues()
{
	mutex.lock();
	{
		for (auto & controlValue : controlValues)
		{
			setMemf(controlValue.name.c_str(), controlValue.currentX, controlValue.currentY);
		}
	}
	mutex.unlock();
}

void AudioGraph::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	mutex.lock();
	{
		auto & mem = memf[name];
		
		mem.value1 = value1;
		mem.value2 = value2;
		mem.value3 = value3;
		mem.value4 = value4;
	}
	mutex.unlock();
}

AudioGraph::Memf AudioGraph::getMemf(const char * name) const
{
	Memf result;
	
	mutex.lock();
	{
		auto memItr = memf.find(name);
		
		if (memItr != memf.end())
		{
			result = memItr->second;
		}
	}
	mutex.unlock();
	
	return result;
}

void AudioGraph::setMems(const char * name, const char * value)
{
	mutex.lock();
	{
		auto & mem = mems[name];
		
		mem.value = value;
	}
	mutex.unlock();
}

AudioGraph::Mems AudioGraph::getMems(const char * name) const
{
	Mems result;
	
	mutex.lock();
	{
		auto memItr = mems.find(name);
		
		if (memItr != mems.end())
		{
			result = memItr->second;
		}
	}
	mutex.unlock();
	
	return result;
}

void AudioGraph::triggerEvent(const char * event)
{
	mutex.lock();
	{
		triggeredEvents.push_back(event);
	}
	mutex.unlock();
}

//

void createAudioTypeDefinitionLibrary(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	createAudioTypeDefinitionLibrary(typeDefinitionLibrary, g_audioEnumTypeRegistrationList, g_audioNodeTypeRegistrationList);
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
			LOG_DBG("create %s took %.2fms", typeName.c_str(), (t2 - t1) / 1000.0);
		#endif
		
			break;
		}
	}
	
	return audioNode;
}

//

extern void linkAudioNodes();

AudioGraph * constructAudioGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary, AudioGraphGlobals * globals, const bool createdPaused)
{
	linkAudioNodes();
	
	//
	
	AudioGraph * audioGraph = new AudioGraph(globals, createdPaused);
	
#if AUDIO_GRAPH_ENABLE_TIMING
	audioGraph->graph = const_cast<Graph*>(&graph);
#endif
	
	Assert(g_currentAudioGraph == nullptr);
	g_currentAudioGraph = audioGraph;
	
	for (auto & nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		AudioNodeBase * audioNode = createAudioNode(node.id, node.typeName, audioGraph);
		
		Assert(audioNode != nullptr);
		if (audioNode == nullptr)
		{
			LOG_ERR("unable to create node. id=%d, typeName=%s", node.id, node.typeName.c_str());
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
				LOG_ERR("unable to setup link. source node doesn't exist", 0);
			if (dstNodeItr == audioGraph->nodes.end())
				LOG_ERR("unable to setup link. destination node doesn't exist", 0);
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
					LOG_ERR("unable to setup link. input node socket doesn't exist. name=%s, index=%d", link.srcNodeSocketName.c_str(), link.srcNodeSocketIndex);
				if (output == nullptr)
					LOG_ERR("unable to setup link. output node socket doesn't exist. name=%s, index=%d", link.dstNodeSocketName.c_str(), link.dstNodeSocketIndex);
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
	
	for (auto & nodeItr : graph.nodes)
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
		
		for (auto & inputValueItr : node.inputValues)
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
	
	for (auto & audioNodeItr : audioGraph->nodes)
	{
		auto nodeId = audioNodeItr.first;
		auto nodeItr = graph.nodes.find(nodeId);
		auto & node = nodeItr->second;
		auto audioNode = audioNodeItr.second;
		
		audioNode->init(node);
	}
	
	Assert(g_currentAudioGraph == audioGraph);
	g_currentAudioGraph = nullptr;
	
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
	LOG_DBG("filling data cache with path: %s", path);
	
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

//

void drawFilterResponse(const AudioNodeBase * node, const float sx, const float sy)
{
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	{
		setColorf(0, 0, 0, .8f);
		hqFillRoundedRect(0, 0, sx, sy, 4.f);
	}
	hqEnd();
	
	const int kNumSteps = 256;
	float response[kNumSteps];
	
	if (node->getFilterResponse(response, kNumSteps))
	{
		setColor(colorWhite);
		hqBegin(HQ_LINES);
		{
			for (int i = 0; i < kNumSteps - 1; ++i)
			{
				const float x1 = sx * (i + 0) / kNumSteps;
				const float x2 = sx * (i + 1) / kNumSteps;
				const float y1 = (1.f - response[i + 0]) * sy;
				const float y2 = (1.f - response[i + 1]) * sy;
				
				hqLine(x1, y1, 3.f, x2, y2, 3.f);
			}
		}
		hqEnd();
	}
}
