#include "framework.h" // log
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodes/vfxNodeBase.h"

#include "Parse.h"
#include "StringEx.h"

extern VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);
extern VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName, VfxGraph * vfxGraph);

void RealTimeConnection::loadBegin()
{
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
		return;
	
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
	input->disconnect();
	
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
		plug->getRwColor() = Color::fromHex(value.c_str());
		return true;
		
	case kVfxPlugType_Image:
		return false;
	case kVfxPlugType_ImageCpu:
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
			const Color & color = plug->getColor();
			value = color.toHexString(true);
			return true;
		}
	case kVfxPlugType_Image:
		{
			auto image = plug->getImage();
			value = String::FormatC("%d [%d x %d]", image->getTexture(), image->getSx(), image->getSy());
			return true;
		}
	case kVfxPlugType_ImageCpu:
		{
			auto image = plug->getImageCpu();
			if (image == nullptr)
				return false;
			else
			{
				value = String::FormatC("[%d x %d]", image->sx, image->sy);
				return true;
			}
		}
	case kVfxPlugType_Trigger:
		{
			const VfxTriggerData & triggerData = plug->getTriggerData();
			
			if (triggerData.type == kVfxTriggerDataType_Bool ||
				triggerData.type == kVfxTriggerDataType_Int)
			{
				value = String::FormatC("%d", triggerData.asInt());
				return true;
			}
			else if (triggerData.type == kVfxTriggerDataType_Float)
			{
				value = String::FormatC("%f", triggerData.asFloat());
				return true;
			}
			else if (triggerData.type == kVfxTriggerDataType_None)
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
	
	return getPlugValue(output, value);
}

int RealTimeConnection::nodeIsActive(const GraphNodeId nodeId)
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
