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
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "graphEdit.h"

#include "Log.h"
#include "Parse.h"
#include "StringEx.h"
#include <string.h>

//

#if defined(DEBUG)
	#define AUDIOGRAPH_DEBUG_DISCONNECTS 1
#else
	#define AUDIOGRAPH_DEBUG_DISCONNECTS 0 // do not alter
#endif

//

#include "audioNodeBase.h"
#include "Timer.h"

AudioValueHistory::AudioValueHistory()
	: lastRequestTime(0)
	, isValid(false)
{
	memset(samples, 0, sizeof(samples));
}

void AudioValueHistory::provide(const AudioFloat & value)
{
	value.expand();
	
	memmove(samples, samples + AUDIO_UPDATE_SIZE, (kNumSamples - AUDIO_UPDATE_SIZE) * sizeof(float));
	
	memcpy(samples + kNumSamples - AUDIO_UPDATE_SIZE, value.samples, AUDIO_UPDATE_SIZE * sizeof(float));
	
	isValid = true;
}

bool AudioValueHistory::isActive() const
{
	const uint64_t time = g_TimerRT.TimeUS_get();
	
	return (time - lastRequestTime) < 1000 * 1000;
}

//

void AudioValueHistorySet::captureInto(AudioValueHistorySet & dst) const
{
	// add or update audio values
	
	for (auto & audioValueItr : audioValues)
	{
		auto & srcAudioValue = audioValueItr.second;
		auto & dstAudioValue = dst.audioValues[audioValueItr.first];
		
		memcpy(
			dstAudioValue.samples,
			srcAudioValue.samples,
			sizeof(dstAudioValue.samples));
		dstAudioValue.isValid = srcAudioValue.isValid;
	}
	
	// prune out-of-date audio values
	
	for (auto i = dst.audioValues.begin(); i != dst.audioValues.end(); )
	{
		if (audioValues.count(i->first) == 0)
			i = dst.audioValues.erase(i);
		else
			++i;
	}
}

//

#define AUDIO_SCOPE AudioScope audioScope(audioMutex)

struct AudioScope
{
	AudioMutexBase * mutex;
	
	AudioScope(AudioMutexBase * in_mutex)
		: mutex(in_mutex)
	{
		mutex->lock();
	}
	
	~AudioScope()
	{
		mutex->unlock();
	}
};

//

AudioRealTimeConnection::AudioRealTimeConnection(
	AudioGraph *& in_audioGraph,
	AudioGraphContext * in_context,
	AudioValueHistorySet * in_audioValueHistorySet,
	AudioValueHistorySet * in_audioValueHistorySetCapture)
	: GraphEdit_RealTimeConnection()
	, audioGraph(nullptr)
	, audioGraphPtr(nullptr)
	, audioMutex(nullptr)
	, isLoading(false)
	, audioValueHistorySet(nullptr)
	, audioValueHistorySetCapture(nullptr)
	, context(nullptr)
{
	audioGraph = in_audioGraph;
	audioGraphPtr = &in_audioGraph;
	
	audioValueHistorySet = in_audioValueHistorySet;
	audioValueHistorySetCapture = in_audioValueHistorySetCapture;
	
	context = in_context;
}

AudioRealTimeConnection::~AudioRealTimeConnection()
{
}

void AudioRealTimeConnection::updateAudioValues()
{
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;

	AUDIO_SCOPE;
	
	if (audioGraph->currentTickTraversalId < 0)
		return;
	
	for (auto i = audioValueHistorySet->audioValues.begin(); i != audioValueHistorySet->audioValues.end(); )
	{
		auto & socketRef = i->first;
		auto & audioValueHistory = i->second;
		
		if (audioValueHistory.isActive() == false)
		{
			i = audioValueHistorySet->audioValues.erase(i);
		}
		else
		{
			if (socketRef.srcSocketIndex != -1)
			{
				auto nodeItr = audioGraph->nodes.find(socketRef.nodeId);
				
				Assert(nodeItr != audioGraph->nodes.end());
				if (nodeItr == audioGraph->nodes.end())
					continue;
				
				auto node = nodeItr->second;
				
				if (node->lastTickTraversalId == -1)
					continue;
				
				auto input = node->tryGetInput(socketRef.srcSocketIndex);
				
				Assert(input != nullptr);
				if (input == nullptr)
					continue;
				
				if (input->isConnected() && input->type == kAudioPlugType_FloatVec)
				{
					setCurrentAudioGraphTraversalId(node->lastTickTraversalId);
					{
						const AudioFloat & value = input->getAudioFloat();
						
						audioValueHistory.provide(value);
					}
					clearCurrentAudioGraphTraversalId();
				}
			}
			else if (socketRef.dstSocketIndex != -1)
			{
				auto nodeItr = audioGraph->nodes.find(socketRef.nodeId);
				
				Assert(nodeItr != audioGraph->nodes.end());
				if (nodeItr == audioGraph->nodes.end())
					continue;
				
				auto node = nodeItr->second;
				
				if (node->lastTickTraversalId == -1)
					continue;
				
				auto output = node->tryGetOutput(socketRef.dstSocketIndex);
				
				Assert(output != nullptr);
				if (output == nullptr)
					continue;
				
				if (output->isConnected() && output->type == kAudioPlugType_FloatVec)
				{
					setCurrentAudioGraphTraversalId(node->lastTickTraversalId);
					{
						const AudioFloat & value = output->getAudioFloat();
						
						audioValueHistory.provide(value);
					}
					clearCurrentAudioGraphTraversalId();
				}
			}
			
			++i;
		}
	}
}

void AudioRealTimeConnection::captureAudioValues()
{
	AUDIO_SCOPE;
	
	audioValueHistorySet->captureInto(*audioValueHistorySetCapture);
}

void AudioRealTimeConnection::loadBegin()
{
	audioMutex->lock();
	
	isLoading = true;
	
	delete audioGraph;
	audioGraph = nullptr;
	*audioGraphPtr = nullptr;
	
	audioValueHistorySet->clear();
	audioValueHistorySetCapture->clear();
}

void AudioRealTimeConnection::loadEnd(GraphEdit & graphEdit)
{
	audioGraph = constructAudioGraph(*graphEdit.graph, graphEdit.typeDefinitionLibrary, context, false);
	*audioGraphPtr = audioGraph;
	
	isLoading = false;
	
	audioMutex->unlock();
}

void AudioRealTimeConnection::nodeAdd(const GraphNode & node)
{
	if (isLoading)
		return;
	
	//LOG_DBG("nodeAdd");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(node.id);
	
	Assert(nodeItr == audioGraph->nodes.end());
	if (nodeItr != audioGraph->nodes.end())
		return;
	
	//
	
	AUDIO_SCOPE;
	
	pushAudioGraph(audioGraph);
	{
		AudioNodeBase * audioNode = createAudioNode(node.id, node.typeName, audioGraph);
	
		if (audioNode != nullptr)
		{
			audioNode->initSelf(node);
			
			audioGraph->nodes[node.id] = audioNode;
		}
	}
	popAudioGraph();
}

void AudioRealTimeConnection::nodeInit(const GraphNode & node)
{
	if (isLoading)
		return;
	
	//LOG_DBG("nodeInit");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(node.id);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	AudioNodeBase * audioNode = nodeItr->second;
	
	//
	
	AUDIO_SCOPE;
	
	pushAudioGraph(audioGraph);
	{
		audioNode->init(node);
	}
	popAudioGraph();
}

void AudioRealTimeConnection::nodeRemove(const GraphNodeId nodeId)
{
	if (isLoading)
		return;
	
	//LOG_DBG("nodeRemove");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto * node = nodeItr->second;
	
#if AUDIOGRAPH_DEBUG_DISCONNECTS
	// all links should be removed at this point. there should be no remaining predeps or connected nodes
	Assert(node->predeps.empty());
	
	// ensure all inputs are disconnected
	for (auto & input : node->inputs)
		Assert(!input.isConnected() || input.mem == input.immediateMem);
	for (auto & i : audioGraph->nodes)
	{
		auto * foreignNode = i.second;
		// ensure there are no foreign node inputs pointing to our memory
		for (auto & foreignInput : foreignNode->inputs)
		{
			for (auto & output : node->outputs)
				Assert(foreignInput.mem != output.mem);
		}
		// ensure there are no foreign node outputs referenced by our inputs
		for (auto & foreignOutput : foreignNode->outputs)
		{
			for (auto & input : node->inputs)
				Assert(foreignOutput.mem != input.mem);
		}
	}
#endif
	
	AUDIO_SCOPE;
	
	auto oldAudioGraph = g_currentAudioGraph;
	g_currentAudioGraph = audioGraph;
	{
		node->shut();
		
		delete node;
		node = nullptr;
	}
	g_currentAudioGraph = oldAudioGraph;
	
	audioGraph->nodes.erase(nodeItr);
	
	for (auto audioValueItr = audioValueHistorySet->audioValues.begin(); audioValueItr != audioValueHistorySet->audioValues.end(); )
	{
		auto & socketRef = audioValueItr->first;
		
		if (socketRef.nodeId == nodeId)
		{
			audioValueItr = audioValueHistorySet->audioValues.erase(audioValueItr);
		}
		else
		{
			audioValueItr++;
		}
	}
}

void AudioRealTimeConnection::linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return;
	
	//LOG_DBG("linkAdd");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto srcNodeItr = audioGraph->nodes.find(srcNodeId);
	auto dstNodeItr = audioGraph->nodes.find(dstNodeId);
	
	Assert(srcNodeItr != audioGraph->nodes.end() && dstNodeItr != audioGraph->nodes.end());
	if (srcNodeItr == audioGraph->nodes.end() || dstNodeItr == audioGraph->nodes.end())
	{
		if (srcNodeItr == audioGraph->nodes.end())
			LOG_ERR("source node doesn't exist");
		if (dstNodeItr == audioGraph->nodes.end())
			LOG_ERR("destination node doesn't exist");
		
		return;
	}

	auto srcNode = srcNodeItr->second;
	auto dstNode = dstNodeItr->second;
	
	auto input = srcNode->tryGetInput(srcSocketIndex);
	auto output = dstNode->tryGetOutput(dstSocketIndex);
	
	Assert(input != nullptr && output != nullptr);
	if (input == nullptr || output == nullptr)
	{
		if (input == nullptr)
			LOG_ERR("input node socket doesn't exist");
		if (output == nullptr)
			LOG_ERR("output node socket doesn't exist");
		
		return;
	}
	
	AUDIO_SCOPE;
	
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
		triggerTarget.srcSocketIndex = srcSocketIndex;
		triggerTarget.dstSocketIndex = dstSocketIndex;
		
		dstNode->triggerTargets.push_back(triggerTarget);
	}
}

void AudioRealTimeConnection::linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return;
	
	//LOG_DBG("linkRemove");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto srcNodeItr = audioGraph->nodes.find(srcNodeId);
	auto dstNodeItr = audioGraph->nodes.find(dstNodeId);
	
	Assert(srcNodeItr != audioGraph->nodes.end() && dstNodeItr != audioGraph->nodes.end());
	if (srcNodeItr == audioGraph->nodes.end() || dstNodeItr == audioGraph->nodes.end())
	{
		if (srcNodeItr == audioGraph->nodes.end())
			LOG_ERR("source node doesn't exist");
		if (dstNodeItr == audioGraph->nodes.end())
			LOG_ERR("destination node doesn't exist");
		
		return;
	}
	
	auto srcNode = srcNodeItr->second;
	auto dstNode = dstNodeItr->second;
	
	AUDIO_SCOPE;
	
	{
		// attempt to remove dst node from predeps
		
		bool foundPredep = false;
		
		for (auto i = srcNode->predeps.begin(); i != srcNode->predeps.end(); ++i)
		{
			AudioNodeBase * audioNode = *i;
			
			if (audioNode == dstNode)
			{
				srcNode->predeps.erase(i);
				foundPredep = true;
				break;
			}
		}
		
		Assert(foundPredep);
	}
	
	{
		auto input = srcNode->tryGetInput(srcSocketIndex);
		auto output = dstNode->tryGetOutput(dstSocketIndex);
		
		Assert(input != nullptr && input->isConnected());
		Assert(output != nullptr);
		
		if (input != nullptr && output != nullptr)
		{
			input->disconnect(output->mem);
		}
	}
	
	// if this is a link hooked up to a trigger, remove the TriggerTarget from dstNode
	
	auto input = srcNode->tryGetInput(srcSocketIndex);
	
	if (input != nullptr && input->type == kAudioPlugType_Trigger)
	{
		bool foundTriggerTarget = false;
		
		for (auto triggerTargetItr = dstNode->triggerTargets.begin(); triggerTargetItr != dstNode->triggerTargets.end(); ++triggerTargetItr)
		{
			auto & triggerTarget = *triggerTargetItr;
			
			if (triggerTarget.srcNode == srcNode && triggerTarget.srcSocketIndex == srcSocketIndex)
			{
				dstNode->triggerTargets.erase(triggerTargetItr);
				foundTriggerTarget = true;
				break;
			}
		}
		
		Assert(foundTriggerTarget);
	}
}

void AudioRealTimeConnection::setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough)
{
	if (isLoading)
		return;
	
	//LOG_DBG("setNodeIsPassthrough called for nodeId=%d, isPassthrough=%d", int(nodeId), int(isPassthrough));
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	AUDIO_SCOPE;
	
	node->isPassthrough = isPassthrough;
}

bool AudioRealTimeConnection::setPlugValue(AudioGraph * audioGraph, AudioPlug * plug, const std::string & value)
{
	switch (plug->type)
	{
	case kAudioPlugType_None:
		return false;
		
	case kAudioPlugType_Bool:
		return Parse::Bool(value, plug->getRwBool());
	
	case kAudioPlugType_Int:
		return Parse::Int32(value, plug->getRwInt());
			
	case kAudioPlugType_Float:
		return Parse::Float(value, plug->getRwFloat());
		
	case kAudioPlugType_String:
		plug->getRwString() = value;
		return true;
		
	case kAudioPlugType_PcmData:
		return false;
		
	case kAudioPlugType_FloatVec:
		{
			float value_asFloat;
			if (!Parse::Float(value, value_asFloat))
				return false;
			plug->getRwAudioFloat().setScalar(value_asFloat);
			return true;
		}

	case kAudioPlugType_Trigger:
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

bool AudioRealTimeConnection::getPlugValue(AudioGraph * audioGraph, AudioPlug * plug, std::string & value)
{
	switch (plug->type)
	{
	case kAudioPlugType_None:
		return false;
		
	case kAudioPlugType_Bool:
		value = String::ToString(plug->getBool());
		return true;
	case kAudioPlugType_Int:
		value = String::ToString(plug->getInt());
		return true;
	case kAudioPlugType_Float:
		value = String::FormatC("%f", plug->getFloat());
		return true;
	case kAudioPlugType_String:
		value = plug->getString();
		return true;
	case kAudioPlugType_PcmData:
		return false;
	case kAudioPlugType_FloatVec:
		setCurrentAudioGraphTraversalId(audioGraph->currentTickTraversalId);
		{
			value = String::FormatC("%f", plug->getAudioFloat().getScalar());
		}
		clearCurrentAudioGraphTraversalId();
		return true;
	case kAudioPlugType_Trigger:
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

void AudioRealTimeConnection::setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value)
{
	if (isLoading)
		return;
	
	//LOG_DBG("setSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	AUDIO_SCOPE;
	
	if (input->isConnected() && input->immediateMem != nullptr && input->immediateMem == input->mem)
	{
		setPlugValue(audioGraph, input, value);
	}
	else
	{
		audioGraph->connectToInputLiteral(*input, value);
	}
}

bool AudioRealTimeConnection::getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value)
{
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return false;
	
	if (input->isConnected() == false)
		return false;
	
	AUDIO_SCOPE;
	
	return getPlugValue(audioGraph, input, value);
}

void AudioRealTimeConnection::setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value)
{
	if (isLoading)
		return;
	
	//LOG_DBG("setDstSocketValue called for nodeId=%d, dstSocket=%s", int(nodeId), dstSocketName.c_str());
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto output = node->tryGetOutput(dstSocketIndex);
	
	Assert(output != nullptr);
	if (output == nullptr)
		return;
	
	AUDIO_SCOPE;
	
	setPlugValue(audioGraph, output, value);
}

bool AudioRealTimeConnection::getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value)
{
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto output = node->tryGetOutput(dstSocketIndex);
	
	Assert(output != nullptr);
	if (output == nullptr)
		return false;
	
	AUDIO_SCOPE;
	
	return getPlugValue(audioGraph, output, value);
}

void AudioRealTimeConnection::clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
{
	if (isLoading)
		return;
	
	//LOG_DBG("clearSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	if (input->isConnected())
	{
		AUDIO_SCOPE;
		
		if (input->mem == input->immediateMem)
		{
			input->immediateMem = nullptr;
			
			input->disconnect();
		}
		else
		{
			input->immediateMem = nullptr;
		}
	}
}

bool AudioRealTimeConnection::getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels)
{
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return false;
	
	if (input->isConnected() == false)
		return false;
	
	// note : this method operates on a copy of the audio data, which is why
	//        we don't have to use a mutex
	
	if (input->type == kAudioPlugType_FloatVec)
	{
		AudioValueHistory_SocketRef ref;
		ref.nodeId = nodeId;
		ref.srcSocketIndex = srcSocketIndex;
		
		// check if there's historic data readily available
		
		auto & history_capture = audioValueHistorySetCapture->audioValues[ref];
		
		if (history_capture.isValid)
		{
			AUDIO_SCOPE;
			
			// historic data is available. make sure it stays available, by
			// updating the last request time
			
			auto & history = audioValueHistorySet->audioValues[ref];
			history.lastRequestTime = g_TimerRT.TimeUS_get();
		}
		else
		{
			AUDIO_SCOPE;
			
			// no historic data is available (yet). add a new history item to the
			// history set actively maintained on the audio thread. this will
			// ensure historic data will become available more efficiently through
			// the capture process, so we can hit the fast path above
			
			auto & history = audioValueHistorySet->audioValues[ref];
			history.lastRequestTime = g_TimerRT.TimeUS_get();
			
			setCurrentAudioGraphTraversalId(audioGraph->currentTickTraversalId);
			const AudioFloat & value = input->getAudioFloat();
			clearCurrentAudioGraphTraversalId();
			
			// make the data available
			
			history_capture.provide(value);
		}
		
		channels.addChannel(history_capture.samples, history_capture.kNumSamples, true);
			
		return true;
	}
	else
	{
		return false;
	}
}

bool AudioRealTimeConnection::getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels)
{
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto output = node->tryGetOutput(dstSocketIndex);
	
	Assert(output != nullptr);
	if (output == nullptr)
		return false;
	
	// note : this method operates on a copy of the audio data, which is why
	//        we don't have to use a mutex
	
	if (output->type == kAudioPlugType_FloatVec)
	{
		AudioValueHistory_SocketRef ref;
		ref.nodeId = nodeId;
		ref.dstSocketIndex = dstSocketIndex;
		
		// check if there's historic data readily available
		
		auto & history_capture = audioValueHistorySetCapture->audioValues[ref];
		
		if (history_capture.isValid)
		{
			AUDIO_SCOPE;
			
			// historic data is available. make sure it stays available, by
			// updating the last request time
			
			auto & history = audioValueHistorySet->audioValues[ref];
			history.lastRequestTime = g_TimerRT.TimeUS_get();
		}
		else
		{
			AUDIO_SCOPE;
			
			// no historic data is available (yet). add a new history item to the
			// history set actively maintained on the audio thread. this will
			// ensure historic data will become available more efficiently through
			// the capture process, so we can hit the fast path above
			
			auto & history = audioValueHistorySet->audioValues[ref];
			history.lastRequestTime = g_TimerRT.TimeUS_get();
			
			setCurrentAudioGraphTraversalId(audioGraph->currentTickTraversalId);
			const AudioFloat & value = output->getAudioFloat();
			clearCurrentAudioGraphTraversalId();
			
			// make the data available
			
			history_capture.provide(value);
		}
		
		channels.addChannel(history_capture.samples, history_capture.kNumSamples, true);
		
		return true;
	}
	else
	{
		return false;
	}
}

void AudioRealTimeConnection::handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
{
	if (isLoading)
		return;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	node->editorIsTriggered = true;
	
	AUDIO_SCOPE;
	
	g_currentAudioGraph = audioGraph;
	
	node->queueTrigger(srcSocketIndex);
	
	g_currentAudioGraph = nullptr;
}

bool AudioRealTimeConnection::getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines)
{
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	AudioNodeDescription d;
	
	AUDIO_SCOPE;
	
	node->getDescription(d);
	
	if (!d.lines.empty())
		d.add("");
	
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	d.add("tick: %.3fms", node->tickTimeAvg / 1000.0);
#endif
	
	std::swap(lines, d.lines);
	
	return true;
}

int AudioRealTimeConnection::getNodeActivity(const GraphNodeId nodeId)
{
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	AUDIO_SCOPE;
	
	int result = 0;
	
	if (node->lastTickTraversalId == audioGraph->currentTickTraversalId)
		result |= kActivity_Continuous;
	
	if (node->editorIsTriggered)
	{
		result |= kActivity_OneShot;
		
		node->editorIsTriggered = false;
	}
	
	return result;
}

int AudioRealTimeConnection::getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return false;
	
	return false;
}

int AudioRealTimeConnection::getNodeCpuHeatMax() const
{
	return 0;
}

int AudioRealTimeConnection::getNodeCpuTimeUs(const GraphNodeId nodeId) const
{
#if ENABLE_AUDIOGRAPH_CPU_TIMING
	if (isLoading)
		return false;
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return false;
	
	AUDIO_SCOPE;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	return node->tickTimeAvg;
#else
	return false;
#endif
}
