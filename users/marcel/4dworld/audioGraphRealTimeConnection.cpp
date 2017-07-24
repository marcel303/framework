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

#include "framework.h" // log
#include "audioGraph.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"

#include "Parse.h"
#include "StringEx.h"

#include <SDL2/SDL.h>

//

#include "audioNodeBase.h"
#include "soundmix.h"
#include "Timer.h"

struct AudioValueHistory
{
	static const int kHistorySize = 8;
	static const int kNumSamples = AUDIO_UPDATE_SIZE * kHistorySize;
	
	GraphNodeId nodeId;
	int srcSocketIndex;
	int dstSocketIndex;
	
	uint64_t lastUpdateTime;
	
	float samples[kNumSamples];
	
	AudioValueHistory()
		: nodeId(kGraphNodeIdInvalid)
		, srcSocketIndex(-1)
		, dstSocketIndex(-1)
		, lastUpdateTime(0)
		, samples()
	{
	}
	
	void provide(const AudioFloat & value)
	{
		value.expand();
		
		memmove(samples, samples + AUDIO_UPDATE_SIZE, (kNumSamples - AUDIO_UPDATE_SIZE) * sizeof(float));
		
		memcpy(samples + kNumSamples - AUDIO_UPDATE_SIZE, value.samples, AUDIO_UPDATE_SIZE * sizeof(float));
	}
	
	bool isActive() const
	{
		const uint64_t time = g_TimerRT.TimeUS_get();
		
		return (time - lastUpdateTime) < 1000 * 1000;
	}
};

struct SocketRef
{
	GraphNodeId nodeId;
	int srcSocketIndex;
	int dstSocketIndex;
	
	SocketRef()
		: nodeId(kGraphNodeIdInvalid)
		, srcSocketIndex(-1)
		, dstSocketIndex(-1)
	{
	}
	
	bool operator<(const SocketRef & other) const
	{
		if (nodeId != other.nodeId)
			return nodeId < other.nodeId;
		else if (srcSocketIndex != other.srcSocketIndex)
			return srcSocketIndex < other.srcSocketIndex;
		else
			return dstSocketIndex < other.dstSocketIndex;
	}
};

struct AudioValueHistorySet
{
	std::map<SocketRef, AudioValueHistory> s_audioValues;
};

//

#define AUDIO_SCOPE AudioScope audioScope(audioMutex)

struct AudioScope
{
	SDL_mutex * mutex;
	
	AudioScope(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
		Assert(mutex != nullptr);
		SDL_LockMutex(mutex);
	}
	
	~AudioScope()
	{
		Assert(mutex != nullptr);
		SDL_UnlockMutex(mutex);
	}
};

//

AudioRealTimeConnection::AudioRealTimeConnection()
	: GraphEdit_RealTimeConnection()
	, audioGraph(nullptr)
	, audioGraphPtr(nullptr)
	, audioMutex(nullptr)
	, isLoading(false)
	, audioValueHistorySet(nullptr)
{
	audioValueHistorySet = new AudioValueHistorySet();
}

AudioRealTimeConnection::~AudioRealTimeConnection()
{
	delete audioValueHistorySet;
	audioValueHistorySet = nullptr;
}

void AudioRealTimeConnection::updateAudioValues()
{
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;

	AUDIO_SCOPE;
	
	for (auto i = audioValueHistorySet->s_audioValues.begin(); i != audioValueHistorySet->s_audioValues.end(); )
	{
		auto & socketRef = i->first;
		auto & audioValueHistory = i->second;
		
		if (audioValueHistory.isActive() == false)
		{
			i = audioValueHistorySet->s_audioValues.erase(i);
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
				
				auto input = node->tryGetInput(socketRef.srcSocketIndex);
				
				Assert(input != nullptr);
				if (input == nullptr)
					continue;
				
				if (input->isConnected() && input->type == kAudioPlugType_FloatVec)
				{
					const AudioFloat & value = input->getAudioFloat();
					
					audioValueHistory.provide(value);
				}
			}
			else if (socketRef.dstSocketIndex != -1)
			{
				auto nodeItr = audioGraph->nodes.find(socketRef.nodeId);
				
				Assert(nodeItr != audioGraph->nodes.end());
				if (nodeItr == audioGraph->nodes.end())
					continue;
				
				auto node = nodeItr->second;
				
				auto output = node->tryGetOutput(socketRef.dstSocketIndex);
				
				Assert(output != nullptr);
				if (output == nullptr)
					continue;
				
				if (output->isConnected() && output->type == kAudioPlugType_FloatVec)
				{
					const AudioFloat & value = output->getAudioFloat();
					
					audioValueHistory.provide(value);
				}
			}
			
			++i;
		}
	}
}

void AudioRealTimeConnection::loadBegin()
{
	Assert(audioMutex != nullptr);
	SDL_LockMutex(audioMutex);
	
	isLoading = true;
	
	delete audioGraph;
	audioGraph = nullptr;
	*audioGraphPtr = nullptr;
}

void AudioRealTimeConnection::loadEnd(GraphEdit & graphEdit)
{
	audioGraph = constructAudioGraph(*graphEdit.graph, graphEdit.typeDefinitionLibrary);
	*audioGraphPtr = audioGraph;
	
	isLoading = false;
	
	Assert(audioMutex != nullptr);
	SDL_UnlockMutex(audioMutex);
}

void AudioRealTimeConnection::nodeAdd(const GraphNodeId nodeId, const std::string & typeName)
{
	if (isLoading)
		return;
	
	//logDebug("nodeAdd");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr == audioGraph->nodes.end());
	if (nodeItr != audioGraph->nodes.end())
		return;
	
	//
	
	GraphNode node;
	node.id = nodeId;
	node.typeName = typeName;
	
	//
	
	AUDIO_SCOPE;
	
	AudioNodeBase * audioNode = createAudioNode(nodeId, typeName, audioGraph);
	
	if (audioNode == nullptr)
		return;
	
	audioNode->initSelf(node);
	
	audioGraph->nodes[node.id] = audioNode;
	
	//
	
	audioNode->init(node);
}

void AudioRealTimeConnection::nodeRemove(const GraphNodeId nodeId)
{
	if (isLoading)
		return;
	
	//logDebug("nodeRemove");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto nodeItr = audioGraph->nodes.find(nodeId);
	
	Assert(nodeItr != audioGraph->nodes.end());
	if (nodeItr == audioGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	// all links should be removed at this point. there should be no remaining predeps or connected nodes
	Assert(node->predeps.empty());
	//for (auto & input : node->inputs)
	//	Assert(!input.isConnected()); // may be a literal value node with a non-accounted for (in the graph) connection when created directly from socket value
	// todo : iterate all other nodes, to ensure there are no nodes with references back to this node?
	
	AUDIO_SCOPE;
	
	delete node;
	node = nullptr;
	
	audioGraph->nodes.erase(nodeItr);
	
	for (auto audioValueItr = audioValueHistorySet->s_audioValues.begin(); audioValueItr != audioValueHistorySet->s_audioValues.end(); )
	{
		auto & socketRef = audioValueItr->first;
		
		if (socketRef.nodeId == nodeId)
		{
			audioValueItr = audioValueHistorySet->s_audioValues.erase(audioValueItr);
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
	
	//logDebug("linkAdd");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto srcNodeItr = audioGraph->nodes.find(srcNodeId);
	auto dstNodeItr = audioGraph->nodes.find(dstNodeId);
	
	Assert(srcNodeItr != audioGraph->nodes.end() && dstNodeItr != audioGraph->nodes.end());
	if (srcNodeItr == audioGraph->nodes.end() || dstNodeItr == audioGraph->nodes.end())
	{
		if (srcNodeItr == audioGraph->nodes.end())
			logError("source node doesn't exist");
		if (dstNodeItr == audioGraph->nodes.end())
			logError("destination node doesn't exist");
		
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
			logError("input node socket doesn't exist");
		if (output == nullptr)
			logError("output node socket doesn't exist");
		
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
	
	//logDebug("linkRemove");
	
	Assert(audioGraph != nullptr);
	if (audioGraph == nullptr)
		return;
	
	auto srcNodeItr = audioGraph->nodes.find(srcNodeId);
	
	Assert(srcNodeItr != audioGraph->nodes.end());
	if (srcNodeItr == audioGraph->nodes.end())
		return;
	
	auto srcNode = srcNodeItr->second;
	
	auto input = srcNode->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	AUDIO_SCOPE;
	
	Assert(input->isConnected());
	
	if (input->type == kAudioPlugType_FloatVec)
	{
		auto dstNodeItr = audioGraph->nodes.find(dstNodeId);
		
		Assert(dstNodeItr != audioGraph->nodes.end());
		if (dstNodeItr == audioGraph->nodes.end())
			return;
		
		auto dstNode = dstNodeItr->second;
		
		auto output = dstNode->tryGetOutput(dstSocketIndex);
		
		Assert(output != nullptr);
		if (output == nullptr)
			return;
		
		bool removed = false;
		for (auto arrayItr = input->floatArray.array.begin(); arrayItr != input->floatArray.array.end(); ++arrayItr)
		{
			if (*arrayItr == output->mem)
			{
				arrayItr = input->floatArray.array.erase(arrayItr);
				removed = true;
				break;
			}
		}
		Assert(removed);
	}
	else
	{
		input->disconnect();
	}
	
	// todo : we should reconnect with the node socket's editor value when set here
	
	{
		// attempt to remove dst node from predeps
		
		auto dstNodeItr = audioGraph->nodes.find(dstNodeId);
		
		Assert(dstNodeItr != audioGraph->nodes.end());
		if (dstNodeItr != audioGraph->nodes.end())
		{
			auto dstNode = dstNodeItr->second;
			
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
	}
	
	// if this is a link hooked up to a trigger, remove the TriggerTarget from dstNode
	
	if (input->type == kAudioPlugType_Trigger)
	{
		auto dstNodeItr = audioGraph->nodes.find(dstNodeId);
		
		Assert(dstNodeItr != audioGraph->nodes.end());
		if (dstNodeItr != audioGraph->nodes.end())
		{
			auto dstNode = dstNodeItr->second;
			
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
}

void AudioRealTimeConnection::setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough)
{
	if (isLoading)
		return;
	
	//logDebug("setNodeIsPassthrough called for nodeId=%d, isPassthrough=%d", int(nodeId), int(isPassthrough));
	
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

bool AudioRealTimeConnection::setPlugValue(AudioPlug * plug, const std::string & value)
{
	switch (plug->type)
	{
	case kAudioPlugType_None:
		return false;
		
	case kAudioPlugType_Bool:
		plug->getRwBool() = Parse::Bool(value);
		return true;
	
	case kAudioPlugType_Int:
		plug->getRwInt() = Parse::Int32(value);
		return true;
			
	case kAudioPlugType_Float:
		plug->getRwFloat() = Parse::Float(value);
		return true;
		
	case kAudioPlugType_String:
		plug->getRwString() = value;
		return true;
		
	case kAudioPlugType_PcmData:
		return false;
		
	case kAudioPlugType_FloatVec:
		plug->getRwAudioFloat().setScalar(Parse::Float(value));
		return true;

	case kAudioPlugType_Trigger:
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

bool AudioRealTimeConnection::getPlugValue(AudioPlug * plug, std::string & value)
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
		value = String::FormatC("%f", plug->getAudioFloat().getScalar());
		return true;
	case kAudioPlugType_Trigger:
		{
			const AudioTriggerData & triggerData = plug->getTriggerData();
			
			if (triggerData.type == kAudioTriggerDataType_Float)
			{
				value = String::FormatC("%f", triggerData.asFloat());
				return true;
			}
			else if (triggerData.type == kAudioTriggerDataType_None)
			{
				return false;
			}
			else
			{
				Assert(false);
				return false;
			}
		}
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

void AudioRealTimeConnection::setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value)
{
	if (isLoading)
		return;
	
	//logDebug("setSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
	
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
	
	if (input->isConnected())
	{
		setPlugValue(input, value);
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
	
	return getPlugValue(input, value);
}

void AudioRealTimeConnection::setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value)
{
	if (isLoading)
		return;
	
	//logDebug("setDstSocketValue called for nodeId=%d, dstSocket=%s", int(nodeId), dstSocketName.c_str());
	
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
	
	setPlugValue(output, value);
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
	
	return getPlugValue(output, value);
}

void AudioRealTimeConnection::clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
{
	if (isLoading)
		return;
	
	//logDebug("clearSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
	
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
		// check if this link is connected to a literal value
		
		bool isImmediate = false;
		
		for (auto & i : audioGraph->valuesToFree)
		{
			if (i.mem == input->mem)
				isImmediate = true;
				
			for (auto & a : input->floatArray.array)
				if (i.mem == a)
					isImmediate = true;
		}
		
		if (isImmediate)
		{
			AUDIO_SCOPE;
			
			input->disconnect();
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
	
	if (input->type == kAudioPlugType_FloatVec)
	{
		AUDIO_SCOPE;
		
		SocketRef ref;
		ref.nodeId = nodeId;
		ref.srcSocketIndex = srcSocketIndex;
		
		auto & history = audioValueHistorySet->s_audioValues[ref];
		history.lastUpdateTime = g_TimerRT.TimeUS_get();
		
		channels.addChannel(history.samples, history.kNumSamples, true);
		
		// todo : let channel own data
		
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
	
	if (output->type == kAudioPlugType_FloatVec)
	{
		AUDIO_SCOPE;
		
		SocketRef ref;
		ref.nodeId = nodeId;
		ref.dstSocketIndex = dstSocketIndex;
		
		auto & history = audioValueHistorySet->s_audioValues[ref];
		history.lastUpdateTime = g_TimerRT.TimeUS_get();
		
		channels.addChannel(history.samples, history.kNumSamples, true);
		
		// todo : let channel own data
		
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
	
	AudioTriggerData data;
	data.setFloat(0.f);
	
	g_currentAudioGraph = audioGraph;
	
	node->handleTrigger(srcSocketIndex, data);
	
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
	
	// todo : restore getDescription !
	
	AUDIO_SCOPE;
	
	//node->getDescription(d);
	
	if (!d.lines.empty())
		d.add("");
	
	d.add("tick: %.3fms", node->tickTimeAvg / 1000.0);
	
	std::swap(lines, d.lines);
	
	return true;
}

int AudioRealTimeConnection::nodeIsActive(const GraphNodeId nodeId)
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
	
	if (node->lastTickTraversalId + 1 == audioGraph->nextTickTraversalId)
		result |= kActivity_Continuous;
	
	if (node->editorIsTriggered)
	{
		result |= kActivity_OneShot;
		
		node->editorIsTriggered = false;
	}
	
	return result;
}

int AudioRealTimeConnection::linkIsActive(const GraphLinkId linkId)
{
	if (isLoading)
		return false;
	
	return false;
}

int AudioRealTimeConnection::getNodeCpuHeatMax() const
{
	return 1000 * 2;
}

int AudioRealTimeConnection::getNodeCpuTimeUs(const GraphNodeId nodeId) const
{
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
}
