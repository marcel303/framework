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

//

AudioGraph::AudioGraph()
	: nodes()
	, displayNodeId(kGraphNodeIdInvalid)
	, nextTickTraversalId(0)
	, nextDrawTraversalId(0)
	, graph(nullptr)
	, valuesToFree()
	, time(0.0)
{
}

AudioGraph::~AudioGraph()
{
	destroy();
}

void AudioGraph::destroy()
{
	displayNodeId = kGraphNodeIdInvalid;
	
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
			delete (AudioValue*)i.mem;
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
		
		input.connectTo(value, kAudioPlugType_Bool);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Bool, value));
	}
	else if (input.type == kAudioPlugType_Int)
	{
		int * value = new int();
		
		*value = Parse::Int32(inputValue);
		
		input.connectTo(value, kAudioPlugType_Int);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Int, value));
	}
	else if (input.type == kAudioPlugType_Float)
	{
		float * value = new float();
		
		*value = Parse::Float(inputValue);
		
		input.connectTo(value, kAudioPlugType_Float);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_Float, value));
	}
	else if (input.type == kAudioPlugType_String)
	{
		std::string * value = new std::string();
		
		*value = inputValue;
		
		input.connectTo(value, kAudioPlugType_String);
		
		valuesToFree.push_back(AudioGraph::ValueToFree(AudioGraph::ValueToFree::kType_String, value));
	}
	else if (input.type == kAudioPlugType_AudioValue)
	{
		const float scalarValue = Parse::Float(inputValue);
		
		AudioValue * value = new AudioValue(scalarValue);
		
		input.connectTo(value, kAudioPlugType_AudioValue);
		
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
	
	// use traversalId, start update at display node
	
	if (displayNodeId != kGraphNodeIdInvalid)
	{
		auto nodeItr = nodes.find(displayNodeId);
		Assert(nodeItr != nodes.end());
		if (nodeItr != nodes.end())
		{
			auto node = nodeItr->second;
			
			AudioNodeDisplay * displayNode = static_cast<AudioNodeDisplay*>(node);
			
			displayNode->traverseTick(nextTickTraversalId, dt);
		}
	}
	
	// process nodes that aren't connected to the display node

	// todo : perhaps process unconnected nodes as islands, following predeps ?
	
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
	
	//
	
	g_currentAudioGraph = nullptr;
}

void AudioGraph::draw(AudioOutputChannel * outputChannels, const int numOutputChannels) const
{
	audioCpuTimingBlock(AudioGraph_Draw);
	
	Assert(g_currentAudioGraph == nullptr);
	g_currentAudioGraph = const_cast<AudioGraph*>(this);
	
	// start traversal at the display node and traverse to leafs following predeps and and back up the tree again to draw
	
	if (displayNodeId != kGraphNodeIdInvalid)
	{
		auto nodeItr = nodes.find(displayNodeId);
		Assert(nodeItr != nodes.end());
		if (nodeItr != nodes.end())
		{
			auto node = nodeItr->second;
			
			AudioNodeDisplay * displayNode = static_cast<AudioNodeDisplay*>(node);
			
			displayNode->outputChannelL = numOutputChannels >= 1 ? &outputChannels[0] : nullptr;
			displayNode->outputChannelR = numOutputChannels >= 2 ? &outputChannels[1] : nullptr;
			{
				displayNode->traverseDraw(nextDrawTraversalId);
			}
			displayNode->outputChannelL = nullptr;
			displayNode->outputChannelR = nullptr;
		}
	}
	
	++nextDrawTraversalId;
	
	g_currentAudioGraph = nullptr;
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
	
	if (typeName == "display")
	{
		Assert(audioNode != nullptr);
		if (audioNode != nullptr)
		{
			// fixme : move display node id handling out of here. remove nodeId and audioGraph passed in to this function
			Assert(audioGraph->displayNodeId == kGraphNodeIdInvalid);
			audioGraph->displayNodeId = nodeId;
		}
	}
	
	return audioNode;
}

//

AudioGraph * constructAudioGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	AudioGraph * audioGraph = new AudioGraph();
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		if (node.nodeType != kGraphNodeType_Regular)
		{
			continue;
		}
		
		if (node.isEnabled == false)
		{
			continue;
		}
		
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
