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
#include "vfxNodeBase.h"

#include "Parse.h"
#include "StringEx.h"

#define VFXGRAPH_DEBUG_PREDEPS 0

void RealTimeConnection::loadBegin()
{
	Assert(g_currentVfxGraph == nullptr);
	
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
	
	VfxNodeBase * vfxNode = createVfxNode(nodeId, typeName);
	
	if (vfxNode == nullptr)
	{
		vfxGraph->nodesFailedToCreate.insert(nodeId);
		
		return;
	}
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = vfxGraph;
	{
		vfxNode->id = nodeId;
		
		vfxNode->initSelf(node);
		
		vfxGraph->nodes[node.id] = vfxNode;
		
		//
		
		vfxNode->init(node);
	}
	g_currentVfxGraph = nullptr;
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
#if defined(DEBUG) && 0
	for (auto & input : node->inputs)
	{
		Assert(!input.isConnected()); // may be a literal value node with a non-accounted for (in the graph) connection when created directly from socket value
	}
	// todo : iterate all other nodes, to ensure there are no nodes with references back to this node?
#endif
	
	auto oldVfxGraph = g_currentVfxGraph;
	g_currentVfxGraph = vfxGraph;
	{
		delete node;
		node = nullptr;
	}
	g_currentVfxGraph = oldVfxGraph;
	
	vfxGraph->nodes.erase(nodeItr);
	
	Assert(vfxGraph->displayNodeIds.count(nodeId) == 0);
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
	
	// add a dynamic link if either the src or dst socket is dynamic
	
	const int numSrcStaticInputs = srcNode->inputs.size() - srcNode->dynamicInputs.size();
	const int numDstStaticInputs = dstNode->outputs.size() - dstNode->dynamicOutputs.size();
	const bool isDynamicLink = srcSocketIndex >= numSrcStaticInputs || dstSocketIndex >= numDstStaticInputs;
	
	if (isDynamicLink)
	{
		logDebug("adding dynamic link");
		
		VfxDynamicLink link;
		
		link.linkId = linkId;
		link.srcNodeId = srcNodeId;
		if (srcSocketIndex < numSrcStaticInputs)
			link.srcSocketIndex = srcSocketIndex;
		else
			link.srcSocketName = srcNode->dynamicInputs[srcSocketIndex - numSrcStaticInputs].name;
		
		link.dstNodeId = dstNodeId;
		if (dstSocketIndex < numDstStaticInputs)
			link.dstSocketIndex = dstSocketIndex;
		else
			link.dstSocketName = dstNode->dynamicOutputs[dstSocketIndex - numDstStaticInputs].name;
		
		vfxGraph->dynamicData->links.push_back(link);
	}
	
	// link the input to the output, if possible
	
	auto input = srcNode->tryGetInput(srcSocketIndex);
	auto output = dstNode->tryGetOutput(dstSocketIndex);
	
	if (input != nullptr && output != nullptr)
	{
		input->connectTo(*output);
	}
	else if (isDynamicLink)
	{
		// the dynamic inputs and/or outputs may be added later
	}
	else
	{
		Assert(input != nullptr && output != nullptr);
		
		if (input == nullptr)
			logError("input node socket doesn't exist");
		if (output == nullptr)
			logError("output node socket doesn't exist");
	}
	
	// note : this may add the same node multiple times to the list of predeps. note that this
	//        is ok as nodes will be traversed once through the travel id + it works nicely
	//        with the live connection as we can just remove the predep and still have one or
	//        references to the predep if the predep was referenced more than once
	srcNode->predeps.push_back(dstNode);
	
	// if this is a trigger, add a trigger target to dstNode
	
	if (output && output->type == kVfxPlugType_Trigger)
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
	
	{
		// attempt to remove dst node from predeps
		
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
	
	// todo : we should reconnect with the node socket's editor value when set here
	
	// if this is a link hooked up to a trigger, remove the TriggerTarget from dstNode
	
	auto input = srcNode->tryGetInput(srcSocketIndex);
	
	if (input != nullptr && input->type == kVfxPlugType_Trigger)
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
	
	// remove a dynamic link if either the src or dst socket is dynamic
	
	{
		for (auto i = vfxGraph->dynamicData->links.begin(); i != vfxGraph->dynamicData->links.end(); ++i)
		{
			auto & link = *i;
			
			if (link.linkId == linkId)
			{
				logDebug("removing dynamic link");
				
				vfxGraph->dynamicData->links.erase(i);
				break;
			}
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

	for (auto & link : vfxGraph->dynamicData->links)
	{
		if (link.linkId == linkId)
		{
			link.params[name] = value;
			
			logDebug("setLinkParameter: set dynamic link parameter. count=%d", link.params.size());
		}
	}
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

	for (auto & link : vfxGraph->dynamicData->links)
	{
		if (link.linkId == linkId)
		{
			link.params.erase(name);
			
			logDebug("clearLinkParameter: cleared dynamic link parameter. count=%d", link.params.size());
		}
	}
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

bool RealTimeConnection::setPlugValue(VfxGraph * vfxGraph, VfxPlug * plug, const std::string & value)
{
	switch (plug->type)
	{
	case kVfxPlugType_None:
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
		
	case kVfxPlugType_Channel:
		{
			VfxChannel & channel = plug->getRwChannel();
			
			float * data;
			int dataSize;
			VfxChannelData::parse(value.c_str(), data, dataSize);
			
			delete [] channel.data;
			channel.data = nullptr;
			
			channel.setData(data, false, dataSize);
			
			return true;
		}
		
	case kVfxPlugType_Trigger:
		return false;
		
	case kVfxPlugType_Draw:
		return false;
	}
	
	Assert(false); // all cases should be handled explicitly
	return false;
}

bool RealTimeConnection::getPlugValue(VfxGraph * vfxGraph, VfxPlug * plug, std::string & value)
{
	switch (plug->type)
	{
	case kVfxPlugType_None:
		return false;
		
	case kVfxPlugType_Bool:
		value = String::ToString(plug->getBool());
		return true;
	case kVfxPlugType_Int:
		value = String::ToString(plug->getInt());
		return true;
	case kVfxPlugType_Float:
		Assert(g_currentVfxGraph == nullptr);
		g_currentVfxGraph = vfxGraph;
		{
			value = String::FormatC("%f", plug->getFloat());
		}
		g_currentVfxGraph = nullptr;
		return true;
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
	case kVfxPlugType_Channel:
		{
			auto channel = plug->getChannel();
			if (channel == nullptr)
				return false;
			else
			{
				if (channel->sy > 1)
					value = String::FormatC("[%d x %d]", channel->sx, channel->sy);
				else
					value = String::FormatC("[%d]", channel->size);
				return true;
			}
		}
	case kVfxPlugType_Trigger:
		return false;
	case kVfxPlugType_Draw:
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
	
	const int numStaticInputs = node->inputs.size() - node->dynamicInputs.size();
	const bool isDynamicInput = srcSocketIndex >= numStaticInputs;
	
	if (isDynamicInput)
	{
		bool found = false;
		
		for (auto & inputSocketValue : vfxGraph->dynamicData->inputSocketValues)
		{
			if (inputSocketValue.nodeId == nodeId && inputSocketValue.socketName == srcSocketName)
			{
				logDebug("updating dynamic input socket value. socketName=%d, socketValue=%s",
					srcSocketName.c_str(),
					value.c_str());
				
				Assert(found == false);
				found = true;
				
				inputSocketValue.value = value;
			}
		}
		
		if (found == false)
		{
			logDebug("adding dynamic input socket value. socketName=%d, socketValue=%s",
				srcSocketName.c_str(),
				value.c_str());
			
			VfxDynamicInputSocketValue inputSocketValue;
			inputSocketValue.nodeId = nodeId;
			inputSocketValue.socketName = srcSocketName;
			inputSocketValue.value = value;
			
			vfxGraph->dynamicData->inputSocketValues.push_back(inputSocketValue);
		}
	}
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(isDynamicInput || input != nullptr);
	if (input != nullptr)
	{
		if (input->isConnected() && input->immediateMem != nullptr && input->immediateMem == input->mem)
		{
			setPlugValue(vfxGraph, input, value);
		}
		else
		{
			vfxGraph->connectToInputLiteral(*input, value);
		}
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
	
	input->referencedByRealTimeConnectionTick = vfxGraph->currentTickTraversalId;
	
	return getPlugValue(vfxGraph, input, value);
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
	
	setPlugValue(vfxGraph, output, value);
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
	
	output->referencedByRealTimeConnectionTick = vfxGraph->currentTickTraversalId;
	
	return getPlugValue(vfxGraph, output, value);
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
	
	const int numStaticInputs = node->inputs.size() - node->dynamicInputs.size();
	const bool isDynamicInput = srcSocketIndex >= numStaticInputs;
	
	if (isDynamicInput)
	{
		for (auto i = vfxGraph->dynamicData->inputSocketValues.begin(); i != vfxGraph->dynamicData->inputSocketValues.end(); ++i)
		{
			auto & inputSocketValue = *i;
			
			if (inputSocketValue.nodeId == nodeId && inputSocketValue.socketName == srcSocketName)
			{
				logDebug("removing dynamic input socket value");
				
				vfxGraph->dynamicData->inputSocketValues.erase(i);
				break;
			}
		}
	}
	
	auto input = node->tryGetInput(srcSocketIndex);
	
	Assert(isDynamicInput || input != nullptr);
	if (input != nullptr && input->isConnected())
	{
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
	
	input->referencedByRealTimeConnectionTick = vfxGraph->currentTickTraversalId;
	
	auto inputChannel = input->getChannel();
	
	for (int y = 0; y < inputChannel->sy; ++y)
		channels.addChannel(inputChannel->data + y * inputChannel->sx, inputChannel->sx, inputChannel->continuous);
	
	//channels.addChannel(inputChannel->data, inputChannel->sx * inputChannel->sy, inputChannel->continuous);
	
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

	output->referencedByRealTimeConnectionTick = vfxGraph->currentTickTraversalId;
	
	auto outputChannel = output->getChannel();
	
	for (int y = 0; y < outputChannel->sy; ++y)
		channels.addChannel(outputChannel->data + y * outputChannel->sx, outputChannel->sx, outputChannel->continuous);
	
	//channels.addChannel(outputChannel->data, outputChannel->sx * outputChannel->sy, outputChannel->continuous);
	
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
	
	node->editorIsTriggeredTick = vfxGraph->currentTickTraversalId;;
	
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
		Assert(vfxGraph->nodesFailedToCreate.count(nodeId) != 0 || nodeItr != vfxGraph->nodes.end());
		
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
		Assert(vfxGraph->nodesFailedToCreate.count(nodeId) != 0 || nodeItr != vfxGraph->nodes.end());
		
		return false;
	}
	
	auto node = nodeItr->second;
	
	VfxNodeDescription d;
	
	node->getDescription(d);
	
	if (!d.lines.empty())
		d.add("");
	
#if ENABLE_VFXGRAPH_CPU_TIMING
	d.add("tick: %.3fms", node->tickTimeAvg / 1000.0);
	d.add("draw: %.3fms", node->drawTimeAvg / 1000.0);
#endif
#if ENABLE_VFXGRAPH_GPU_TIMING
	d.add("gpu: %.3fms", node->gpuTimeAvg / 1000000.0);
#endif
	
#if VFXGRAPH_DEBUG_PREDEPS
	d.add("predeps: %d", node->predeps.size());
	for (auto & predep : node->predeps)
		d.add("predep: %p", predep);
	d.add("self: %p", this);
#endif
	
	if (node->editorIssue.empty() == false)
	{
		d.newline();
		d.add("issue: %s", node->editorIssue.c_str());
	}
	
	std::swap(lines, d.lines);
	
	return true;
}

int RealTimeConnection::getNodeActivity(const GraphNodeId nodeId)
{
	if (isLoading)
		return false;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	if (nodeItr == vfxGraph->nodes.end())
	{
		Assert(vfxGraph->nodesFailedToCreate.count(nodeId) != 0 || nodeItr != vfxGraph->nodes.end());
		
		return false;
	}
	
	auto node = nodeItr->second;
	
	int result = 0;
	
	if (node->lastDrawTraversalId + 1 == vfxGraph->nextDrawTraversalId)
		result |= kActivity_Continuous;
	
	if (node->editorIsTriggeredTick == vfxGraph->currentTickTraversalId)
	{
		result |= kActivity_OneShot;
	}
	
	return result;
}

int RealTimeConnection::getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
{
	if (isLoading)
		return false;
	
	auto dstNodeItr = vfxGraph->nodes.find(dstNodeId);
	
	Assert(vfxGraph->nodesFailedToCreate.count(dstNodeId) != 0 || dstNodeItr != vfxGraph->nodes.end());
	if (dstNodeItr == vfxGraph->nodes.end())
		return false;
	
	auto dstNode = dstNodeItr->second;
	
#if defined(DEBUG)
	if (dstSocketIndex == -1)
	{
		// the socket index may only be -1 if this is a dynamic link
		bool isDynamic = false;
		for (auto & dlink : vfxGraph->dynamicData->links)
			if (dlink.linkId == linkId)
				isDynamic = true;
		Assert(isDynamic);
		return false;
	}
#endif

	auto dstInput = dstNode->tryGetOutput(dstSocketIndex);
	
	Assert(dstInput != nullptr);
	if (dstInput == nullptr)
		return false;
	
	// todo : check if the link was triggered
	
	bool result = false;
	
	if (dstInput->editorIsTriggeredTick == vfxGraph->currentTickTraversalId)
	{
		result = true;
	}
	
	return result;
}

static std::string vfxPlugTypeToValueTypeName(const VfxPlugType plugType)
{
	switch (plugType)
	{
		case kVfxPlugType_None:
			Assert(false);
			return "";
		case kVfxPlugType_Bool:
			return "bool";
		case kVfxPlugType_Int:
			return "int";
		case kVfxPlugType_Float:
			return "float";
		case kVfxPlugType_String:
			return "string";
		case kVfxPlugType_Color:
			return "color";
		case kVfxPlugType_Image:
			return "image";
		case kVfxPlugType_ImageCpu:
			return "image_cpu";
		case kVfxPlugType_Channel:
			return "channel";
		case kVfxPlugType_Trigger:
			return "trigger";
		case kVfxPlugType_Draw:
			return "draw";
	}
	
	Assert(false);
	return "";
}

bool RealTimeConnection::getNodeDynamicSockets(const GraphNodeId nodeId, std::vector<DynamicInput> & inputs, std::vector<DynamicOutput> & outputs) const
{
	if (isLoading)
		return false;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(vfxGraph->nodesFailedToCreate.count(nodeId) != 0 || nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return false;
	
	auto node = nodeItr->second;
	
	if (node->dynamicInputs.empty() && node->dynamicOutputs.empty())
	{
		return false;
	}
	else
	{
		inputs.resize(node->dynamicInputs.size());
		outputs.resize(node->dynamicOutputs.size());
		
		int index = 0;
		
		for (auto & input : node->dynamicInputs)
		{
			inputs[index].name = input.name;
			inputs[index].typeName = vfxPlugTypeToValueTypeName(input.type);
			
			index++;
		}
		
		index = 0;
		
		for (auto & output : node->dynamicOutputs)
		{
			outputs[index].name = output.name;
			outputs[index].typeName = vfxPlugTypeToValueTypeName(output.type);
			
			index++;
		}
		
		return true;
	}
}

int RealTimeConnection::getNodeCpuHeatMax() const
{
	return 1000 * (1000 / 30);
}

int RealTimeConnection::getNodeCpuTimeUs(const GraphNodeId nodeId) const
{
#if ENABLE_VFXGRAPH_CPU_TIMING
	if (isLoading)
		return 0;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return 0;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(vfxGraph->nodesFailedToCreate.count(nodeId) != 0 || nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return 0;
	
	auto node = nodeItr->second;
	
	return node->tickTimeAvg;
#else
	return 0;
#endif
}

int RealTimeConnection::getNodeGpuHeatMax() const
{
	return 1000 * (1000 / 30);
}

int RealTimeConnection::getNodeGpuTimeUs(const GraphNodeId nodeId) const
{
#if ENABLE_VFXGRAPH_GPU_TIMING
	if (isLoading)
		return 0;
	
	Assert(vfxGraph != nullptr);
	if (vfxGraph == nullptr)
		return 0;
	
	auto nodeItr = vfxGraph->nodes.find(nodeId);
	
	Assert(vfxGraph->nodesFailedToCreate.count(nodeId) != 0 || nodeItr != vfxGraph->nodes.end());
	if (nodeItr == vfxGraph->nodes.end())
		return 0;
	
	auto node = nodeItr->second;
	
	return node->gpuTimeAvg;
#else
	return 0;
#endif
}
