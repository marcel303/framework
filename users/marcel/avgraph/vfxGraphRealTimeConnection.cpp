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
	
	if (vfxGraph->displayNodeIds.count(nodeId) != 0)
	{
		vfxGraph->displayNodeIds.erase(nodeId);
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
		
		if (removed && input->floatArray.elems.empty())
		{
			Assert(input->mem == nullptr);
			input->memType = kVfxPlugType_None;
		}
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
	
	input->referencedByRealTimeConnectionTick = g_currentVfxGraph->nextTickTraversalId;
	
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

	output->referencedByRealTimeConnectionTick = g_currentVfxGraph->nextTickTraversalId;
	
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

bool RealTimeConnection::getNodeIssues(const GraphNodeId nodeId, std::vector<std::string> & issues)
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
	
	if (node->editorIssue.empty())
	{
		return false;
	}
	else
	{
		issues.push_back(node->editorIssue);
		
		return true;
	}
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
	
	if (node->editorIssue.empty() == false)
	{
		d.newline();
		d.add("issue: %s", node->editorIssue.c_str());
	}
	
	std::swap(lines, d.lines);
	
	return true;
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

int RealTimeConnection::linkIsActive(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return false;
	
	auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
	
	Assert(dstNodeItr != vfxGraph->nodes.end());
	if (dstNodeItr == vfxGraph->nodes.end())
		return false;
	
	auto dstNode = dstNodeItr->second;
	
	auto dstInput = dstNode->tryGetOutput(dstSocketIndex);
	
	Assert(dstInput != nullptr);
	if (dstInput == nullptr)
		return false;
	
	// todo : check if the link was triggered
	
	bool result = false;
	
	if (dstInput->editorIsTriggered)
	{
		// fixme : setting this to false here won't work correctly when there's multiply outgoing connections for this socket. we should report true for all of these connections. we should, instead, clear all of these flags prior to processing the VfxGraph
		
		dstInput->editorIsTriggered = false;
		
		result = true;
	}
	
	return result;
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
