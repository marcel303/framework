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
#include "audioNodeBase.h"
#include "audioVoiceManager.h"
#include "graph_typeDefinitionLibrary.h"
#include "Log.h"
#include "Parse.h"
#include "soundmix.h"
#include <algorithm>

#if AUDIO_GRAPH_ENABLE_TIMING
	#include "Timer.h"
#endif

//

AUDIO_THREAD_LOCAL AudioGraph * g_currentAudioGraph = nullptr;

//

AudioGraph::AudioGraph(AudioGraphContext * in_context, const bool in_isPaused)
	: isPaused(in_isPaused)
	, rampDownRequested(false)
	, rampDown(false)
	, rampedDown(false)
	, nodes()
	, currentTickTraversalId(0)
#if AUDIO_GRAPH_ENABLE_TIMING
	, graph(nullptr)
#endif
	, valuesToFree()
	, time(0.0)
	, context(nullptr)
{
	context = in_context;
}

AudioGraph::~AudioGraph()
{
	destroy();
}

void AudioGraph::destroy()
{
	pushAudioGraph(this);
	{
		for (auto & nodeItr : nodes)
		{
			AudioNodeBase * node = nodeItr.second;
			
			node->shut();
		}
	}
	popAudioGraph();
	
	// all of the voice nodes should have freed their voices when shut was called
	Assert(audioVoices.empty());
	
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
	
	pushAudioGraph(this);
	{
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
	}
	popAudioGraph();
	
#if AUDIO_GRAPH_ENABLE_TIMING
	graph = nullptr;
#endif
}

void AudioGraph::connectToInputLiteral(AudioPlug & input, const std::string & inputValue)
{
	if (input.type == kAudioPlugType_Bool)
	{
		bool * value = new bool();
		
		*value = Parse::Bool(inputValue);
		
		input.connectToImmediate(value, kAudioPlugType_Bool);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Bool, value));
	}
	else if (input.type == kAudioPlugType_Int)
	{
		int * value = new int();
		
		*value = Parse::Int32(inputValue);
		
		input.connectToImmediate(value, kAudioPlugType_Int);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Int, value));
	}
	else if (input.type == kAudioPlugType_Float)
	{
		float * value = new float();
		
		*value = Parse::Float(inputValue);
		
		input.connectToImmediate(value, kAudioPlugType_Float);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Float, value));
	}
	else if (input.type == kAudioPlugType_String)
	{
		std::string * value = new std::string();
		
		*value = inputValue;
		
		input.connectToImmediate(value, kAudioPlugType_String);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_String, value));
	}
	else if (input.type == kAudioPlugType_FloatVec)
	{
		const float scalarValue = Parse::Float(inputValue);
		
		AudioFloat * value = new AudioFloat(scalarValue);
		
		input.connectToImmediate(value, kAudioPlugType_FloatVec);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_AudioValue, value));
	}
	else
	{
		LOG_WRN("cannot instantiate literal for non-supported type %d, value=%s", input.type, inputValue.c_str());
	}
}

bool AudioGraph::allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex)
{
	if (context->voiceMgr->allocVoice(voice, source, name, doRamping, rampDelay, rampTime, channelIndex))
	{
		Assert(audioVoices.count(voice) == 0);
		audioVoices.insert(voice);
		return true;
	}
	else
	{
		return false;
	}
}

void AudioGraph::freeVoice(AudioVoice *& voice)
{
	if (voice != nullptr)
	{
		Assert(audioVoices.count(voice) != 0);
		audioVoices.erase(voice);
		
		context->voiceMgr->freeVoice(voice);
	}
}

void AudioGraph::lockControlValues()
{
	context->mutex_mem->lock();
}

void AudioGraph::unlockControlValues()
{
	context->mutex_mem->unlock();
}

void AudioGraph::syncMainToAudio(const float dt)
{
	Assert(context->mainThreadId.checkThreadId() == false);
	
	context->mutex_mem->lock();
	{
		// update control values
		
		for (auto & controlValue : controlValues)
		{
			const float retain = powf(controlValue.smoothness, dt);
			
			controlValue.currentX = controlValue.currentX * retain + controlValue.desiredX * (1.f - retain);
			controlValue.currentY = controlValue.currentY * retain + controlValue.desiredY * (1.f - retain);
		}
		
		// export control values
		
		for (auto & controlValue : controlValues)
		{
			setMemf(
				controlValue.name.c_str(),
				controlValue.currentX,
				controlValue.currentY);
		}
		
		// synchronize memory from main -> audio
		
		for (auto & memf_itr : memf)
		{
			auto & memf = memf_itr.second;
			
			memf.syncMainToAudio();
		}
		
		for (auto & mems_itr : mems)
		{
			auto & mems = mems_itr.second;
			
			mems.syncMainToAudio();
		}
	
		// capture and reset triggered events
		
		std::swap(
			triggeredEvents_audioThread,
			triggeredEvents_mainThread);
		triggeredEvents_mainThread.clear();
	}
	context->mutex_mem->unlock();
}

void AudioGraph::tickAudio(const float dt, const bool in_syncMainToAudio)
{
	audioCpuTimingBlock(AudioGraph_Tick);
	
	if (isPaused)
		return;
	
	pushAudioGraph(this);
	{
		if (rampDownRequested)
			rampDown = true;
		
		if (in_syncMainToAudio)
		{
			syncMainToAudio(dt);
		}
		
		// process nodes
		
		++currentTickTraversalId;
		
		setCurrentAudioGraphTraversalId(currentTickTraversalId);
		
		for (auto & i : nodes)
		{
			AudioNodeBase * node = i.second;
			
			if (node->lastTickTraversalId != currentTickTraversalId)
			{
				node->traverseTick(currentTickTraversalId, dt);
			}
		}
		
		clearCurrentAudioGraphTraversalId();
		
		//
		
		time += dt;
		
		//
		
		if (rampDown)
		{
			bool isRampedDown = true;
			
			for (AudioVoice * voice : audioVoices)
			{
				Assert(voice->rampInfo.ramp == false);
				
				isRampedDown &= voice->rampInfo.rampValue == 0.f;
			}
			
			if (isRampedDown)
				rampedDown = true;
		}
	}
	popAudioGraph();
}

void AudioGraph::setFlag(const char * name, const bool value)
{
	// thread: main, audio
	
	context->mutex_mem->lock();
	{
		if (value)
			activeFlags.insert(name);
		else
			activeFlags.erase(name);
	}
	context->mutex_mem->unlock();
}

void AudioGraph::resetFlag(const char * name)
{
	// thread: main, audio
	
	context->mutex_mem->lock();
	{
		activeFlags.erase(name);
	}
	context->mutex_mem->unlock();
}

bool AudioGraph::isFlagSet(const char * name) const
{
	// thread: main, audio
	
	bool result;
	
	context->mutex_mem->lock();
	{
		result = activeFlags.count(name) != 0;
	}
	context->mutex_mem->unlock();
	
	return result;
}

void AudioGraph::registerMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	context->mutex_reg->lock();
	context->mutex_mem->lock();
	{
		auto mem_itr = memf.find(name);
		
		if (mem_itr == memf.end())
		{
			auto & mem = memf[name];
			
			mem.value1_mainThread = value1;
			mem.value2_mainThread = value2;
			mem.value3_mainThread = value3;
			mem.value4_mainThread = value4;
			
			mem.syncMainToAudio();
		}
	}
	context->mutex_mem->unlock();
	context->mutex_reg->unlock();
}

void AudioGraph::registerMems(const char * name, const char * value)
{
	context->mutex_reg->lock();
	context->mutex_mem->lock();
	{
		auto mem_itr = mems.find(name);
		
		if (mem_itr == mems.end())
		{
			auto & mem = mems[name];
			
			mem.value_mainThread = value;
			
			mem.syncMainToAudio();
		}
	}
	context->mutex_mem->unlock();
	context->mutex_reg->unlock();
}

void AudioGraph::registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY)
{
	context->mutex_reg->lock();
	context->mutex_mem->lock();
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
			
			{
				// note : it won't be safe to reference controlValue after the sort,
				//        so we make sure to let it leave scope before the sort
				
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
			}
			
			// keep the list of control values sorted (for a possible UI)
			
			std::sort(controlValues.begin(), controlValues.end(), [](const AudioControlValue & a, const AudioControlValue & b) { return a.name < b.name; });
			
			// register the memf. if we don't, setMemf would fail!
			
			registerMemf(name, defaultX, defaultY, 0.f, 0.f);
		}
	}
	context->mutex_mem->unlock();
	context->mutex_reg->unlock();
}

void AudioGraph::unregisterControlValue(const char * name)
{
	context->mutex_reg->lock();
	context->mutex_mem->lock();
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
	context->mutex_mem->unlock();
	context->mutex_reg->unlock();
}

void AudioGraph::registerEvent(const char * name)
{
	context->mutex_reg->lock();
	context->mutex_mem->lock();
	{
		bool exists = false;
		
		for (auto & event : events)
		{
			if (event.name == name)
			{
				event.refCount++;
				exists = true;
				break;
			}
		}
		
		if (exists == false)
		{
			events.resize(events.size() + 1);
			
			{
				// note : it won't be safe to reference event after the sort,
				//        so we make sure to let it leave scope before the sort
				
				auto & event = events.back();
				
				event.name = name;
				event.refCount = 1;
			}
			
			std::sort(events.begin(), events.end(), [](const AudioEvent & a, const AudioEvent & b) { return a.name < b.name; });
		}
	}
	context->mutex_mem->unlock();
	context->mutex_reg->unlock();
}

void AudioGraph::unregisterEvent(const char * name)
{
	context->mutex_reg->lock();
	context->mutex_mem->lock();
	{
		bool exists = false;
		
		for (auto eventItr = events.begin(); eventItr != events.end(); ++eventItr)
		{
			auto & event = *eventItr;
			
			if (event.name == name)
			{
				event.refCount--;
				
				if (event.refCount == 0)
				{
					//LOG_DBG("erasing event %s", name);
					
					events.erase(eventItr);
				}
				
				exists = true;
				break;
			}
		}
		
		Assert(exists);
		if (exists == false)
		{
			LOG_WRN("failed to unregister event %s", name);
		}
	}
	context->mutex_mem->unlock();
	context->mutex_reg->unlock();
}

void AudioGraph::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	context->mutex_mem->lock();
	{
		auto mem_itr = memf.find(name);
		
		if (mem_itr != memf.end())
		{
			auto & mem = mem_itr->second;
			
			mem.value1_mainThread = value1;
			mem.value2_mainThread = value2;
			mem.value3_mainThread = value3;
			mem.value4_mainThread = value4;
		}
	}
	context->mutex_mem->unlock();
}

AudioGraph::Memf AudioGraph::getMemf(const char * name, const bool isMainThread) const
{
	Assert(context->mainThreadId.checkThreadId() == isMainThread);

	bool found = false;
	
	Memf result;
	
	context->mutex_mem->lock();
	{
		auto mem_itr = memf.find(name);
		
		if (mem_itr != memf.end())
		{
			auto & mem = mem_itr->second;
			
			result = mem;
			
			found = true;
		}
	}
	context->mutex_mem->unlock();
	
	Assert(found);
	
	return result;
}

void AudioGraph::setMems(const char * name, const char * value)
{
	context->mutex_mem->lock();
	{
		auto mem_itr = mems.find(name);
	
		Assert(mem_itr != mems.end());
		if (mem_itr != mems.end())
		{
			auto & mem = mem_itr->second;
			
			mem.value_mainThread = value;
		}
	}
	context->mutex_mem->unlock();
}

void AudioGraph::getMems(const char * name, std::string & result) const
{
	Assert(context->mainThreadId.checkThreadId() == false);

	context->mutex_mem->lock();
	{
		auto mem_itr = mems.find(name);
		
		Assert(mem_itr != mems.end());
		if (mem_itr != mems.end())
		{
			auto & mem = mem_itr->second;
			
			result = mem.value_audioThread;
		}
		else
		{
			result.clear();
		}
	}
	context->mutex_mem->unlock();
}

void AudioGraph::triggerEvent(const char * event)
{
	Assert(context->mainThreadId.checkThreadId() == true);
	
	context->mutex_mem->lock();
	{
		triggeredEvents_mainThread.insert(event);
	}
	context->mutex_mem->unlock();
}

//

void pushAudioGraph(AudioGraph * audioGraph)
{
	Assert(g_currentAudioGraph == nullptr);
	g_currentAudioGraph = audioGraph;
}

void popAudioGraph()
{
	g_currentAudioGraph = nullptr;
}

//

#include "audioTypeDB.h"

AudioNodeBase * createAudioNode(
	const GraphNodeId nodeId,
	const std::string & typeName,
	AudioGraph * audioGraph)
{
	AudioNodeBase * audioNode = nullptr;
	
	for (AudioNodeTypeRegistration * r = g_audioNodeTypeRegistrationList; r != nullptr; r = r->next)
	{
		if (r->typeName == typeName)
		{
		#if AUDIO_GRAPH_ENABLE_TIMING
			const uint64_t t1 = g_TimerRT.TimeUS_get();
		#endif
		
			audioNode = r->create(r->createData);
			
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

AudioGraph * constructAudioGraph(
	const Graph & graph,
	const Graph_TypeDefinitionLibrary * typeDefinitionLibrary,
	AudioGraphContext * context,
	const bool createdPaused)
{
	AudioGraph * audioGraph = new AudioGraph(context, createdPaused);
	
#if AUDIO_GRAPH_ENABLE_TIMING
	audioGraph->graph = const_cast<Graph*>(&graph);
#endif
	
	pushAudioGraph(audioGraph);
	{
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
					LOG_ERR("unable to setup link. source node doesn't exist");
				if (dstNodeItr == audioGraph->nodes.end())
					LOG_ERR("unable to setup link. destination node doesn't exist");
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
					//        with the real-time editing connection as we can just remove the predep and still
					//        have one or more references to the predep if the predep was referenced more than once
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
	}
	popAudioGraph();
	
	return audioGraph;
}
