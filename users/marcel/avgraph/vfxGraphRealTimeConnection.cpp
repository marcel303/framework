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
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodes/vfxNodeBase.h"

#include "Parse.h"
#include "StringEx.h"

void RealTimeConnection::loadBegin()
{
	Assert(g_currentVfxGraph == vfxGraph);
	g_currentVfxGraph = nullptr;
	
	isLoading = true;
	
	delete vfxGraph;
	vfxGraph = nullptr;
	*vfxGraphPtr = nullptr;
}

void RealTimeConnection::loadEnd(GraphEdit & graphEdit)
{
	vfxGraph = constructVfxGraph(*graphEdit.graph, graphEdit.typeDefinitionLibrary);
	*vfxGraphPtr = vfxGraph;
	
	vfxGraph->graph = graphEdit.graph;
	
	isLoading = false;
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = vfxGraph;
}

void RealTimeConnection::saveBegin(GraphEdit & graphEdit)
{
	for (auto & vfxNodeItr : vfxGraph->nodes)
	{
		const GraphNodeId nodeId = vfxNodeItr.first;
		const VfxNodeBase * vfxNode = vfxNodeItr.second;
		
		auto nodeItr = graphEdit.graph->nodes.find(nodeId);
		
		Assert(nodeItr != graphEdit.graph->nodes.end());
		if (nodeItr != graphEdit.graph->nodes.end())
		{
			GraphNode & node = nodeItr->second;
			
			vfxNode->beforeSave(node);
		}
	}
}

void RealTimeConnection::nodeAdd(const GraphNodeId nodeId, const std::string & typeName)
{
	if (isLoading)
		return;
	
	logDebug("nodeAdd");
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr == vfxGraph->nodes.end());
	if (nodeItr != vfxGraph->nodes.end())
		return;
	
	//
	
	GraphNode node;
	node.id = nodeId;
	node.typeName = typeName;
	
	//
	
	VfxNodeBase * vfxNode = createVfxNode(nodeId, typeName, vfxGraph);
	
	if (vfxNode == nullptr)
	{
		vfxGraph->nodesFailedToCreate.insert(nodeId);
		
		return;
	}
	
	vfxNode->initSelf(node);
	
	vfxGraph->nodes[node.id] = vfxNode;
	
	//
	
	vfxNode->init(node);
}

void RealTimeConnection::nodeRemove(const GraphNodeId nodeId)
{
	if (isLoading)
		return;
	
	logDebug("nodeRemove");
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	// all links should be removed at this point. there should be no remaining predeps or connected nodes
	Assert(node->predeps.empty());
	//for (auto & input : node->inputs)
	//	Assert(!input.isConnected()); // may be a literal value node with a non-accounted for (in the graph) connection when created directly from socket value
	// todo : iterate all other nodes, to ensure there are no nodes with references back to this node?
	
	delete node;
	node = nullptr;
	
	vfxGraph->nodes.erase(nodeItr);
	
	if (nodeId == vfxGraph->displayNodeId)
	{
		vfxGraph->displayNodeId = kGraphNodeIdInvalid;
	}
}

void RealTimeConnection::linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return;
	
	logDebug("linkAdd");
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
	auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
	
	Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
	if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
	{
		if (srcNodeItr == vfxGraph->nodes.end())
			logError("source node doesn't exist");
		if (dstNodeItr == vfxGraph->nodes.end())
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
	
	input->connectTo(*output);
	
	// note : this may add the same node multiple times to the list of predeps. note that this
	//        is ok as nodes will be traversed once through the travel id + it works nicely
	//        with the live connection as we can just remove the predep and still have one or
	//        references to the predep if the predep was referenced more than once
	srcNode->predeps.push_back(dstNode);
	
	// if this is a trigger, add a trigger target to dstNode
	if (output->type == kVfxPlugType_Trigger)
	{
		VfxNodeBase::TriggerTarget triggerTarget;
		triggerTarget.srcNode = srcNode;
		triggerTarget.srcSocketIndex = srcSocketIndex;
		triggerTarget.dstSocketIndex = dstSocketIndex;
		
		dstNode->triggerTargets.push_back(triggerTarget);
	}
}

void RealTimeConnection::linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return;
	
	logDebug("linkRemove");
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
	
	Assert(srcNodeItr != vfxGraph->nodes.end());
	if (srcNodeItr == vfxGraph->nodes.end())
		return;
	
	auto srcNode = srcNodeItr->second;
	
	auto input = srcNode->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	Assert(input->isConnected());
	
	if (input->type == kVfxPlugType_Float)
	{
		auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
		
		Assert(dstNodeItr != vfxGraph->nodes.end());
		if (dstNodeItr == vfxGraph->nodes.end())
			return;
		
		auto dstNode = dstNodeItr->second;
		
		auto output = dstNode->tryGetOutput(dstSocketIndex);
		
		Assert(output != nullptr);
		if (output == nullptr)
			return;
		
		bool removed = false;
		for (auto elemItr = input->floatArray.elems.begin(); elemItr != input->floatArray.elems.end(); )
		{
			if (elemItr->value == output->mem)
			{
				elemItr = input->floatArray.elems.erase(elemItr);
				removed = true;
				break;
			}
			
			++elemItr;
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
		
		auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
		
		Assert(dstNodeItr != vfxGraph->nodes.end());
		if (dstNodeItr != vfxGraph->nodes.end())
		{
			auto dstNode = dstNodeItr->second;
			
			bool foundPredep = false;
			
			for (auto i = srcNode->predeps.begin(); i != srcNode->predeps.end(); ++i)
			{
				VfxNodeBase * vfxNode = *i;
				
				if (vfxNode == dstNode)
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
	
	if (input->type == kVfxPlugType_Trigger)
	{
		auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
		
		Assert(dstNodeItr != vfxGraph->nodes.end());
		if (dstNodeItr != vfxGraph->nodes.end())
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

void RealTimeConnection::setLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name, const std::string & value)
{
	if (isLoading)
		return;
	
	logDebug("setLinkParameter");
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	// get src & dst node. check link type
	
	auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
	auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
	
	Assert(srcNodeItr != vfxGraph->nodes.end());
	Assert(dstNodeItr != vfxGraph->nodes.end());
	
	if (srcNodeItr == vfxGraph->nodes.end())
		return;
	if (dstNodeItr == vfxGraph->nodes.end())
		return;
	
	auto srcNode = srcNodeItr->second;
	auto dstNode = dstNodeItr->second;
	
	//
	
	auto srcSocket = srcNode->tryGetInput(srcSocketIndex);
	auto dstSocket = dstNode->tryGetOutput(dstSocketIndex);
	
	Assert(srcSocket != nullptr);
	Assert(dstSocket != nullptr);
	
	if (srcSocket == nullptr)
		return;
	if (dstSocket == nullptr)
		return;
	
#if EXTENDED_INPUTS
	if (srcSocket->type == kVfxPlugType_Float && dstSocket->type == kVfxPlugType_Float)
	{
		const bool isRemap =
			name == "in.min" ||
			name == "in.max" ||
			name == "out.min" ||
			name == "out.max";
		
		if (isRemap)
		{
			for (auto & elem : srcSocket->floatArray.elems)
			{
				if (elem.value == dstSocket->mem)
				{
					const float inMin = name == "in.min" ? Parse::Float(value) : elem.inMin;
					const float inMax = name == "in.max" ? Parse::Float(value) : elem.inMax;
					const float outMin = name == "out.min" ? Parse::Float(value) : elem.outMin;
					const float outMax = name == "out.max" ? Parse::Float(value) : elem.outMax;
					
					srcSocket->setMap(dstSocket->mem, inMin, inMax, outMin, outMax);
				}
			}
		}
	}
#endif
}

void RealTimeConnection::clearLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name)
{
	if (isLoading)
		return;
	
	logDebug("clearLinkParameter");
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	// get src & dst node. check link type
	
	auto srcNodeItr = vfxGraph->nodes.find(srcNodeId);
	auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
	
	Assert(srcNodeItr != vfxGraph->nodes.end());
	Assert(dstNodeItr != vfxGraph->nodes.end());
	
	if (srcNodeItr == vfxGraph->nodes.end())
		return;
	if (dstNodeItr == vfxGraph->nodes.end())
		return;
	
	auto srcNode = srcNodeItr->second;
	auto dstNode = dstNodeItr->second;
	
	//
	
	auto srcSocket = srcNode->tryGetInput(srcSocketIndex);
	auto dstSocket = dstNode->tryGetOutput(dstSocketIndex);
	
	Assert(srcSocket != nullptr);
	Assert(dstSocket != nullptr);
	
	if (srcSocket == nullptr)
		return;
	if (dstSocket == nullptr)
		return;
	
#if EXTENDED_INPUTS
	if (srcSocket->type == kVfxPlugType_Float && dstSocket->type == kVfxPlugType_Float)
	{
		const bool isRemap =
			name == "in.min" ||
			name == "in.max" ||
			name == "out.min" ||
			name == "out.max";
		
		if (isRemap)
		{
			for (auto & elem : srcSocket->floatArray.elems)
			{
				if (elem.value == dstSocket->mem)
				{
					if (name == "in.min")
						elem.inMin = 0.f;
					if (name == "in.max")
						elem.inMax = 1.f;
					if (name == "out.min")
						elem.outMin = 0.f;
					if (name == "out.max")
						elem.outMax = 1.f;
					
					const bool isDefault =
						elem.inMin == 0.f &&
						elem.inMax == 1.f &&
						elem.outMin == 0.f &&
						elem.outMax == 1.f;
					
					if (isDefault)
					{
						srcSocket->clearMap(dstSocket->mem);
					}
				}
			}
		}
	}
#endif
}

void RealTimeConnection::setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough)
{
	if (isLoading)
		return;
	
	//logDebug("setNodeIsPassthrough called for nodeId=%d, isPassthrough=%d", int(nodeId), int(isPassthrough));
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	node->isPassthrough = isPassthrough;
}

bool RealTimeConnection::setPlugValue(VfxPlug * plug, const std::string & value)
{
	switch (plug->type)
	{
	case kVfxPlugType_None:
		return false;
	case kVfxPlugType_DontCare:
		return false;
		
	case kVfxPlugType_Bool:
		plug->getRwBool() = Parse::Bool(value);
		return true;
	case kVfxPlugType_Int:
		plug->getRwInt() = Parse::Int32(value);
		return true;
	case kVfxPlugType_Float:
		plug->getRwFloat() = Parse::Float(value);
		return true;
		
	case kVfxPlugType_Transform:
		return false;
		
	case kVfxPlugType_String:
		plug->getRwString() = value;
		return true;
	case kVfxPlugType_Color:
		{
			const Color color = Color::fromHex(value.c_str());
			plug->getRwColor() = VfxColor(color.r, color.g, color.b, color.a);
			return true;
		}
		
	case kVfxPlugType_Image:
		return false;
	case kVfxPlugType_ImageCpu:
		return false;
	case kVfxPlugType_Channels:
		return false;
	case kVfxPlugType_Trigger:
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

bool RealTimeConnection::getPlugValue(VfxPlug * plug, std::string & value)
{
	switch (plug->type)
	{
	case kVfxPlugType_None:
		return false;
	case kVfxPlugType_DontCare:
		return false;
		
	case kVfxPlugType_Bool:
		value = String::ToString(plug->getBool());
		return true;
	case kVfxPlugType_Int:
		value = String::ToString(plug->getInt());
		return true;
	case kVfxPlugType_Float:
		value = String::FormatC("%f", plug->getFloat());
		return true;
	case kVfxPlugType_Transform:
		return false;
	case kVfxPlugType_String:
		value = plug->getString();
		return true;
	case kVfxPlugType_Color:
		{
			const VfxColor & vfxColor = plug->getColor();
			const Color color(vfxColor.r, vfxColor.g, vfxColor.b, vfxColor.a);
			value = color.toHexString(true);
			return true;
		}
	case kVfxPlugType_Image:
		{
			auto image = plug->getImage();
			if (image->getTexture() == 0)
				return false;
			else
			{
				value = String::FormatC("%d", image->getTexture());
				return true;
			}
		}
	case kVfxPlugType_ImageCpu:
		{
			auto image = plug->getImageCpu();
			if (image == nullptr)
				return false;
			else
			{
				value = String::FormatC("[%d x %d x %d]", image->sx, image->sy, image->numChannels);
				return true;
			}
		}
	case kVfxPlugType_Channels:
		{
			auto channels = plug->getChannels();
			if (channels == nullptr)
				return false;
			else
			{
				value = String::FormatC("[%d x %d]", channels->numChannels, channels->size);
				return true;
			}
		}
	case kVfxPlugType_Trigger:
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

void RealTimeConnection::setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value)
{
	if (isLoading)
		return;
	
	//logDebug("setSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	if (input->isConnected())
	{
		setPlugValue(input, value);
	}
	else
	{
		vfxGraph->connectToInputLiteral(*input, value);
	}
}

bool RealTimeConnection::getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return false;
	
	if (input->isConnected() == false)
		return false;
	
	input->referencedByRealTimeConnectionTick = g_currentVfxGraph->nextTickTraversalId;
	
	return getPlugValue(input, value);
}

void RealTimeConnection::setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value)
{
	if (isLoading)
		return;
	
	//logDebug("setDstSocketValue called for nodeId=%d, dstSocket=%s", int(nodeId), dstSocketName.c_str());
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto output = node->tryGetOutput(dstSocketIndex);
	
	Assert(output != nullptr);
	if (output == nullptr)
		return;
	
	setPlugValue(output, value);
}

bool RealTimeConnection::getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto output = node->tryGetOutput(dstSocketIndex);
	
	Assert(output != nullptr);
	if (output == nullptr)
		return false;
	
	output->referencedByRealTimeConnectionTick = g_currentVfxGraph->nextTickTraversalId;
	
	return getPlugValue(output, value);
}

void RealTimeConnection::clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
{
	if (isLoading)
		return;
	
	//logDebug("clearSrcSocketValue called for nodeId=%d, srcSocket=%s", int(nodeId), srcSocketName.c_str());
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
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
		
		for (auto & i : vfxGraph->valuesToFree)
		{
			if (i.mem == input->mem)
				isImmediate = true;
			
			for (auto & elem : input->floatArray.elems)
				if (i.mem == elem.value)
					isImmediate = true;
			if (i.mem == input->floatArray.immediateValue)
				isImmediate = true;
		}
		
		if (isImmediate)
		{
			input->disconnect();
		}
	}
}

bool RealTimeConnection::getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return false;
	
	if (input->isConnected() == false)
		return false;
	
	auto inputChannels = input->getChannels();
	
	for (int i = 0; i < inputChannels->numChannels; ++i)
	{
		channels.addChannel(inputChannels->channels[i].data, inputChannels->sx * inputChannels->sy, inputChannels->channels[i].continuous);
	}
	
	return true;
}

bool RealTimeConnection::getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	auto output = node->tryGetOutput(dstSocketIndex);
	
	Assert(output != nullptr);
	if (output == nullptr)
		return false;

	auto outputChannels = output->getChannels();
	
	for (int i = 0; i < outputChannels->numChannels; ++i)
	{
		channels.addChannel(outputChannels->channels[i].data, outputChannels->sx * outputChannels->sy, outputChannels->channels[i].continuous);
	}
	
	return true;
}

void RealTimeConnection::handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
{
	if (isLoading)
		return;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return;
	
	auto node = nodeItr->second;
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(input != nullptr);
	if (input == nullptr)
		return;
	
	node->editorIsTriggered = true;
	
	node->handleTrigger(srcSocketIndex);
}

bool RealTimeConnection::getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	if (nodeItr == vfxGraph->nodes.end())
	{
		if (vfxGraph->nodesFailedToCreate.count(nodeId) == 0)
			Assert(nodeItr != vfxGraph->nodes.end());
		
		return false;
	}
	
	auto node = nodeItr->second;
	
	VfxNodeDescription d;
	
	node->getDescription(d);
	
	if (!d.lines.empty())
		d.add("");
	
	d.add("tick: %.3fms", node->tickTimeAvg / 1000.0);
	d.add("draw: %.3fms", node->drawTimeAvg / 1000.0);
	
	std::swap(lines, d.lines);
	
	return true;
}

#include "../libparticle/ui.h" // todo : remove
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include "vfxTypes.h"

static float screenToBeat(const int x1, const int x2, const int x, const float offset, const float bpm, const float length)
{
	const float numBeats = length * bpm / 60.f;
	
	return saturate((x - x1) / float(x2 - x1)) * numBeats + offset;
}

static float beatToScreen(int x1, int x2, float beat, const float bpm, const float length)
{
	const float numBeats = length * bpm / 60.f;
	
	return x1 + (x2 - x1) * beat / numBeats;
}

static VfxTimeline::Key * findNearestKey(VfxTimeline & timeline, const int x1, const int x2, const float beat, const float maxDeviation)
{
	VfxTimeline::Key * nearestKey = 0;
	float nearestDistance = 0.f;

	for (int i = 0; i < timeline.numKeys; ++i)
	{
		const float dt = beatToScreen(x1, x2, timeline.keys[i].beat, timeline.bpm, timeline.length) - beatToScreen(x1, x2, beat, timeline.bpm, timeline.length);
		
		const float distance = std::sqrtf(dt * dt);

		if (distance < maxDeviation && (distance < nearestDistance || nearestKey == 0))
		{
			nearestKey = &timeline.keys[i];
			nearestDistance = distance;
		}
	}

	return nearestKey;
}

static void resetSelectedKeyUi()
{
	g_menu->getElem("beat").reset();
	g_menu->getElem("id").reset();
}

void doVfxTimeline(VfxTimeline & timeline, int & selectedKeyIndex, const char * name, const float dt)
{
	const int kTimelineHeight = 20;
	const Color kBackgroundFocusColor(0.f, 0.f, 1.f, g_uiState->opacity * .7f);
	const Color kBackgroundColor(0.f, 0.f, 0.f, g_uiState->opacity * .8f);
	
	pushMenu(name);
	
	doBreak();
	
	UiElem & elem = g_menu->getElem("timeline");
	
	enum Vars
	{
		kVar_SelectedKey,
		kVar_IsDragging,
		kVar_DragOffset
	};
	
	VfxTimeline::Key *& selectedKey = elem.getPointer<VfxTimeline::Key*>(kVar_SelectedKey, nullptr);
	bool & isDragging = elem.getBool(kVar_IsDragging, false);
	float & dragOffset = elem.getFloat(kVar_DragOffset, 0.f);
	
	if (selectedKeyIndex >= 0 && selectedKeyIndex < timeline.numKeys)
		selectedKey = &timeline.keys[selectedKeyIndex];
	
	const float kMaxSelectionDeviation = 5;
	
	g_drawX -= g_menu->sx * 2;
	g_menu->sx *= 3;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kTimelineHeight;

	g_drawY += kTimelineHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive && false)
			selectedKey = nullptr;
		else if (elem.hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float beat = screenToBeat(x1, x2, mouse.x, 0.f, timeline.bpm, timeline.length);
				auto key = findNearestKey(timeline, x1, x2, beat, kMaxSelectionDeviation);
				
				if (key == nullptr)
				{
					// insert a new key

					if (timeline.allocKey(key))
					{
						key->beat = beat;
						key->id = 0;

						key = timeline.sortKeys(key);
					}
				}
				
				selectedKey = key;

				if (selectedKey != nullptr)
				{
					isDragging = true;
					dragOffset = key->beat - beat;
				}
				
				resetSelectedKeyUi();
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKey = nullptr;

					// erase key

					const float beat = screenToBeat(x1, x2, mouse.x, 0.f, timeline.bpm, timeline.length);
					auto * key = findNearestKey(timeline, x1, x2, beat, kMaxSelectionDeviation);

					if (key)
					{
						timeline.freeKey(key);
					}
					
					g_menu->getElem("beat").deactivate();
				}
			}
		}

		if (isDragging)
		{
			if (elem.isActive && selectedKey != nullptr && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float beat = screenToBeat(x1, x2, mouse.x, dragOffset, timeline.bpm, timeline.length);

				selectedKey->beat = beat;
				selectedKey = timeline.sortKeys(selectedKey);

				fassert(selectedKey->beat == beat);
				
				resetSelectedKeyUi();
			}
			else
			{
				float beat = selectedKey->beat;
				
				if (!keyboard.isDown(SDLK_LSHIFT) && !keyboard.isDown(SDLK_RSHIFT))
					beat = std::round(beat * 4.f) / 4.f;
				
				selectedKey->beat = beat;
				
				isDragging = false;
			}
		}
		else if (elem.isActive && selectedKey != nullptr)
		{
			if (keyboard.wentDown(SDLK_LEFT, true))
			{
				selectedKey->beat -= 1.f / 4.f;
				if (!keyboard.isDown(SDLK_LSHIFT) && !keyboard.isDown(SDLK_RSHIFT))
					selectedKey->beat = std::round(selectedKey->beat * 4.f) / 4.f;
				selectedKey = timeline.sortKeys(selectedKey);
				resetSelectedKeyUi();
			}
			
			if (keyboard.wentDown(SDLK_RIGHT, true))
			{
				selectedKey->beat += 1.f / 4.f;
				if (!keyboard.isDown(SDLK_LSHIFT) && !keyboard.isDown(SDLK_RSHIFT))
					selectedKey->beat = std::round(selectedKey->beat * 4.f) / 4.f;
				selectedKey = timeline.sortKeys(selectedKey);
				resetSelectedKeyUi();
			}
			
			if (keyboard.wentDown(SDLK_UP, true))
			{
				selectedKey->id++;
				resetSelectedKeyUi();
			}
			
			if (keyboard.wentDown(SDLK_DOWN, true))
			{
				selectedKey->id--;
				resetSelectedKeyUi();
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawUiRectCheckered(cx1, cy1, cx2, cy2, 4.f);
		
		const int numBeatMarkers = int(timeline.length * timeline.bpm / 60.f);
		
		hqBegin(HQ_LINES);
		{
			for (int i = 1; i < numBeatMarkers * 4; ++i)
			{
				if ((i % 4) == 0)
					continue;
				
				const float x = beatToScreen(x1, x2, i / 4.f, timeline.bpm, timeline.length);
				
				setColor(127, 127, 127, 127);
				hqLine(x, y1, 1.2f, x, y2, 1.2f);
			}
			
			for (int i = 1; i < numBeatMarkers; ++i)
			{
				const float x = beatToScreen(x1, x2, i, timeline.bpm, timeline.length);
				
				setColor(255, 127, 0, 127);
				hqLine(x, y1, 1.2f, x, y2, 1.2f);
			}
		}
		hqEnd();
		
		{
			const float beat = screenToBeat(x1, x2, mouse.x, 0.f, timeline.bpm, timeline.length);
			auto key = findNearestKey(timeline, x1, x2, beat, kMaxSelectionDeviation);
			for (int i = 0; i < timeline.numKeys; ++i)
			{
				const float c =
					  !elem.hasFocus ? .5f
					: (key == &timeline.keys[i]) ? 1.f
					: (selectedKey == &timeline.keys[i]) ? .8f
					: .5f;
				const float x = beatToScreen(x1, x2, timeline.keys[i].beat, timeline.bpm, timeline.length);
				const float y = (y1 + y2) / 2.f;
				drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
			}
		}
	}
	
	doTextBox(timeline.length, "length (sec)", 0.f, .3f, false, dt);
	doTextBox(timeline.bpm, "bpm", .3f, .3f, true, dt);
	
	if (selectedKey != nullptr)
	{
		doTextBox(selectedKey->beat, "beat", 0.f, .3f, false, dt);
		doTextBox(selectedKey->id, "id", .3f, .3f, true, dt);
		selectedKey = timeline.sortKeys(selectedKey);
		fassert(selectedKey != nullptr);
	}
	
	doBreak();
	
	g_menu->sx /= 3;
	g_drawX += g_menu->sx * 2;
	
	selectedKeyIndex = selectedKey == nullptr ? -1 : selectedKey - timeline.keys;
	
	popMenu();
}

bool RealTimeConnection::doEditor(std::string & valueText, const std::string & name, const std::string & defaultValue, const bool doActions, const bool doDraw, const float dt)
{
	VfxTimeline timeline;
	
	int selectedKeyIndex = -1;
	
	tinyxml2::XMLDocument d;
	if (d.Parse(valueText.c_str()) == tinyxml2::XML_SUCCESS)
	{
		tinyxml2::XMLElement * e = d.FirstChildElement();
		
		if (e != nullptr)
		{
			selectedKeyIndex = intAttrib(e, "selectedKey", -1);
			
			timeline.load(e);
		}
	}
	
	doVfxTimeline(timeline, selectedKeyIndex, name.c_str(), dt);
	
	tinyxml2::XMLPrinter p;
	p.OpenElement("value");
	p.PushAttribute("selectedKey", selectedKeyIndex);
	timeline.save(&p);
	p.CloseElement();
	
	valueText = p.CStr();
	
	return false;
}

int RealTimeConnection::nodeIsActive(const GraphNodeId nodeId)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	if (nodeItr == vfxGraph->nodes.end())
	{
		if (vfxGraph->nodesFailedToCreate.count(nodeId) == 0)
			Assert(nodeItr != vfxGraph->nodes.end());
		
		return false;
	}
	
	auto node = nodeItr->second;
	
	int result = 0;
	
	if (node->lastDrawTraversalId + 1 == vfxGraph->nextDrawTraversalId)
		result |= kActivity_Continuous;
	
	if (node->editorIsTriggered)
	{
		result |= kActivity_OneShot;
		
		node->editorIsTriggered = false;
	}
	
	return result;
}

int RealTimeConnection::linkIsActive(const GraphLinkId linkId)
{
	if (isLoading)
		return false;
	
	return false;
}

int RealTimeConnection::getNodeCpuHeatMax() const
{
	return 1000 * (1000 / 30);
}

int RealTimeConnection::getNodeCpuTimeUs(const GraphNodeId nodeId) const
{
		if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	return node->tickTimeAvg;
}
