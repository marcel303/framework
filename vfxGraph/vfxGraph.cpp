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

#include "framework.h"
#include "graph_typeDefinitionLibrary.h"
#include "Parse.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxTypes.h"

#if VFX_GRAPH_ENABLE_TIMING
	#include "Timer.h"
#endif

//

VfxGraph * g_currentVfxGraph = nullptr;

Surface * g_currentVfxSurface = nullptr;

//

void MemoryComponent::registerMemf(const char * name, const int numElements)
{
	auto & mem = memf[name];
	
	mem.refCount++;
	
	if (numElements > mem.numElements)
		mem.numElements = numElements;
}

void MemoryComponent::unregisterMemf(const char * name)
{
	auto mem_itr = memf.find(name);
	Assert(mem_itr != memf.end());
	
	mem_itr->second.refCount--;
	
	if (mem_itr->second.refCount == 0)
	{
		memf.erase(mem_itr);
	}
}

void MemoryComponent::setMemf(const char * name, const float value1, const float value2, const float value3, const float value4)
{
	auto mem_itr = memf.find(name);
	
	if (mem_itr != memf.end())
	{
		auto & mem = mem_itr->second;
		
		mem.value = Vec4(value1, value2, value3, value4);
	}
}

bool MemoryComponent::getMemf(const char * name, Vec4 & result) const
{
	auto i = memf.find(name);
	
	if (i == memf.end())
		return false;
	else
	{
		result = i->second.value;
		return true;
	}
}

void MemoryComponent::registerMems(const char * name)
{
	auto & mem = mems[name];
	
	mem.refCount++;
}

void MemoryComponent::unregisterMems(const char * name)
{
	auto mem_itr = mems.find(name);
	Assert(mem_itr != mems.end());
	
	mem_itr->second.refCount--;
	
	if (mem_itr->second.refCount == 0)
	{
		mems.erase(mem_itr);
	}
}

void MemoryComponent::setMems(const char * name, const char * value)
{
	auto mem_itr = mems.find(name);
	
	if (mem_itr != mems.end())
	{
		auto & mem = mem_itr->second;
		
		mem.value = value;
	}
}

bool MemoryComponent::getMems(const char * name, std::string & result) const
{
	auto i = mems.find(name);
	
	if (i == mems.end())
		return false;
	else
	{
		result = i->second.value;
		return true;
	}
}

//

VfxGraph::VfxGraph(VfxGraphContext * in_context)
	: context(nullptr)
	, nodes()
	, dynamicData(nullptr)
	, displayNodeIds()
	, currentTickTraversalId(-1)
	, nextDrawTraversalId(0)
	, valuesToFree()
	, dummySurface(nullptr)
	, memory()
	, sx(0)
	, sy(0)
	, time(0.0)
{
	if (in_context == nullptr)
		in_context = new VfxGraphContext();
	context = in_context;
	context->refCount++;
	
	dynamicData = new VfxDynamicData();
	
	dummySurface = new Surface(1, 1, false, false, true, 1);
}

VfxGraph::~VfxGraph()
{
	destroy();
}

void VfxGraph::destroy()
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
		case ValueToFree::kType_Color:
			delete (Color*)i.mem;
			break;
		case ValueToFree::kType_Channel:
			{
				VfxChannel * channel = (VfxChannel*)i.mem;
				if (channel)
					delete channel->data;
				delete channel;
			}
			break;
		default:
			Assert(false);
			break;
		}
	}
	
	valuesToFree.clear();
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = this;
	
	for (auto & i : nodes)
	{
		VfxNodeBase * node = i.second;
		
	#if VFX_GRAPH_ENABLE_TIMING && 0
		const uint64_t t1 = g_TimerRT.TimeUS_get();
	#endif
		
		delete node;
		node = nullptr;
		
	#if VFX_GRAPH_ENABLE_TIMING && 0
		const uint64_t t2 = g_TimerRT.TimeUS_get();
		auto graphNode = graph->tryGetNode(i.first);
		const std::string typeName = graphNode ? graphNode->typeName : "n/a";
		logDebug("delete %s took %.2fms", typeName.c_str(), (t2 - t1) / 1000.0);
	#endif
	}
	
	Assert(displayNodeIds.empty());
	
	g_currentVfxGraph = nullptr;
	
	delete dummySurface;
	dummySurface = nullptr;
	
	delete dynamicData;
	dynamicData = nullptr;
	
	nodes.clear();
	
	context->refCount--;
	if (context->refCount == 0)
		delete context;
	context = nullptr;
}

void VfxGraph::connectToInputLiteral(VfxPlug & input, const std::string & inputValue)
{
	if (input.type == kVfxPlugType_Bool)
	{
		bool * value = new bool();
		
		*value = Parse::Bool(inputValue);
		
		input.connectToImmediate(value, kVfxPlugType_Bool);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Bool, value));
	}
	else if (input.type == kVfxPlugType_Int)
	{
		int * value = new int();
		
		*value = Parse::Int32(inputValue);
		
		input.connectToImmediate(value, kVfxPlugType_Int);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Int, value));
	}
	else if (input.type == kVfxPlugType_Float)
	{
		float * value = new float();
		
		*value = Parse::Float(inputValue);
		
		input.connectToImmediate(value, kVfxPlugType_Float);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Float, value));
	}
	else if (input.type == kVfxPlugType_String)
	{
		std::string * value = new std::string();
		
		*value = inputValue;
		
		input.connectToImmediate(value, kVfxPlugType_String);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_String, value));
	}
	else if (input.type == kVfxPlugType_Color)
	{
		Color * value = new Color();
		
		*value = Color::fromHex(inputValue.c_str());
		
		input.connectToImmediate(value, kVfxPlugType_Color);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Color, value));
	}
	else if (input.type == kVfxPlugType_Channel)
	{
		VfxChannel * value = new VfxChannel();
		
		float * data;
		int dataSize;
		VfxChannelData::parse(inputValue.c_str(), data, dataSize);
		
		value->setData(data, false, dataSize);
		
		input.connectToImmediate(value, kVfxPlugType_Channel);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Channel, value));
	}
	else
	{
		logWarning("cannot instantiate literal for non-supported type %d, value=%s", input.type, inputValue.c_str());
	}
}

void VfxGraph::tick(const int in_sx, const int in_sy, const float dt)
{
	vfxCpuTimingBlock(VfxGraph_Tick);
	vfxGpuTimingBlock(VfxGraph_Tick);
	
	sx = in_sx;
	sy = in_sy;

	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = this;
	
	// process nodes. start update at display node
	
	++currentTickTraversalId;
	
	if (displayNodeIds.empty() == false)
	{
		for (auto displayNodeId : displayNodeIds)
		{
			auto nodeItr = nodes.find(displayNodeId);
			Assert(nodeItr != nodes.end());
			if (nodeItr != nodes.end())
			{
				auto * node = nodeItr->second;
				
				if (node->isPassthrough)
					continue;
				
				VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
				
				displayNode->traverseTick(currentTickTraversalId, dt);
			}
		}
	}
	
	// process nodes that aren't connected to the display node
	
	for (auto & i : nodes)
	{
		VfxNodeBase * node = i.second;
		
		if (node->lastTickTraversalId != currentTickTraversalId)
		{
			node->traverseTick(currentTickTraversalId, dt);
		}
	}
	
	//
	
	time += dt;
	
	//
	
	g_currentVfxGraph = nullptr;
}

void VfxGraph::draw() const
{
	const GxTextureId texture = traverseDraw();
	
	if (texture != 0)
	{
		gxSetTexture(texture);
		pushBlend(BLEND_OPAQUE);
		setColor(colorWhite);
		drawRect(0, 0, sx, sy);
		popBlend();
		gxSetTexture(0);
	}
}

int VfxGraph::traverseDraw() const
{
	vfxCpuTimingBlock(VfxGraph_Draw);
	vfxGpuTimingBlock(VfxGraph_Draw);
	
	int result = 0;
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = const_cast<VfxGraph*>(this);
	
	Assert(g_currentVfxSurface == nullptr);
	g_currentVfxSurface = nullptr;
	
	// start traversal at the display node and traverse to leafs following predeps and and back up the tree again to draw
	
	VfxNodeDisplay * displayNode = getMainDisplayNode();
	
	if (displayNode != nullptr)
	{
		displayNode->traverseDraw(nextDrawTraversalId);
		
		const VfxImageBase * image = displayNode->getImage();
		
		if (image != nullptr)
		{
			result = image->getTexture();
		}
	}
	
#if 1 // todo : make this depend on whether the graph editor is visible or not ? or whether the node is referenced by the editor ?

	// note : traversal for sub-graphs not connected to the display node should start at nodes without any connected outputs. otherwise we might start in the middle of a sub-graph, resulting in undefined behavior
	
	pushSurface(dummySurface);
	{
		// draw nodes that aren't connected to the display node
		
		Assert(g_currentVfxSurface == nullptr);
		g_currentVfxSurface = dummySurface;
		
		for (auto & i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			if (node->lastDrawTraversalId == nextDrawTraversalId)
				continue;
			
			bool isRootNode = true;
			
			for (auto & output : node->outputs)
				if (output.isReferencedByLink)
					isRootNode = false;
			
			if (isRootNode)
			{
				node->traverseDraw(nextDrawTraversalId);
			}
		}
		
		g_currentVfxSurface = nullptr;
	}
	popSurface();
#endif

	++nextDrawTraversalId;
	
	//
	
	g_currentVfxGraph = nullptr;
	
	return result;
}

VfxNodeDisplay * VfxGraph::getMainDisplayNode() const
{
	for (auto displayNodeId : displayNodeIds)
	{
		auto nodeItr = nodes.find(displayNodeId);
		Assert(nodeItr != nodes.end());
		if (nodeItr != nodes.end())
		{
			auto * node = nodeItr->second;
			
			if (node->isPassthrough)
				continue;
			
			return static_cast<VfxNodeDisplay*>(node);
		}
	}
	
	return nullptr;
}

//

float VfxDynamicLink::floatParam(const char * name, const float defaultValue) const
{
	auto i = params.find(name);
	
	if (i == params.end())
		return defaultValue;
	else
		return Parse::Float(i->second);
}

//

VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName)
{
	VfxNodeBase * vfxNode = nullptr;
	
	for (VfxNodeTypeRegistration * r = g_vfxNodeTypeRegistrationList; r != nullptr; r = r->next)
	{
		if (r->typeName == typeName)
		{
		#if VFX_GRAPH_ENABLE_TIMING
			const uint64_t t1 = g_TimerRT.TimeUS_get();
		#endif
		
			vfxNode = r->create();
			
		#if VFX_GRAPH_ENABLE_TIMING
			const uint64_t t2 = g_TimerRT.TimeUS_get();
			logDebug("create %s took %.2fms", typeName.c_str(), (t2 - t1) / 1000.0);
		#endif
		
			break;
		}
	}
	
	return vfxNode;
}

//

VfxGraph * constructVfxGraph(const Graph & graph, const Graph_TypeDefinitionLibrary * typeDefinitionLibrary, VfxGraphContext * context)
{
	VfxGraph * vfxGraph = new VfxGraph(context);
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = vfxGraph;
	
	for (auto & nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		VfxNodeBase * vfxNode = createVfxNode(node.id, node.typeName);
		
		Assert(vfxNode != nullptr);
		if (vfxNode == nullptr)
		{
			logError("unable to create node. nodeId=%d, typeName=%s", node.id, node.typeName.c_str());
			
			vfxGraph->nodesFailedToCreate.insert(node.id);
		}
		else
		{
			vfxNode->id = node.id;
			
			vfxNode->isPassthrough = node.isPassthrough;
			
			vfxNode->initSelf(node);
			
			vfxGraph->nodes[node.id] = vfxNode;
		}
	}
	
	g_currentVfxGraph = nullptr;
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
		if (link.isEnabled == false)
		{
			continue;
		}
		
		auto srcNodeItr = vfxGraph->nodes.find(link.srcNodeId);
		auto dstNodeItr = vfxGraph->nodes.find(link.dstNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
		{
			if (srcNodeItr == vfxGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == vfxGraph->nodes.end())
				logError("destination node doesn't exist");
		}
		else
		{
			auto srcNode = srcNodeItr->second;
			auto dstNode = dstNodeItr->second;
			
			if (link.isDynamic)
			{
				srcNode->predeps.push_back(dstNode);
				
				VfxDynamicLink dlink;
				dlink.linkId = link.id;
				dlink.srcNodeId = link.srcNodeId;
				dlink.srcSocketIndex = link.srcNodeSocketIndex;
				dlink.srcSocketName = link.srcNodeSocketName;
				dlink.dstNodeId = link.dstNodeId;
				dlink.dstSocketIndex = link.dstNodeSocketIndex;
				dlink.dstSocketName = link.dstNodeSocketName;
				dlink.params = link.params;
				vfxGraph->dynamicData->links.push_back(dlink);
				continue;
			}
			
			auto input = srcNode->tryGetInput(link.srcNodeSocketIndex);
			auto output = dstNode->tryGetOutput(link.dstNodeSocketIndex);
			
			Assert(input != nullptr && output != nullptr);
			if (input == nullptr || output == nullptr)
			{
				if (input == nullptr)
					logError("input node socket doesn't exist. name=%s, index=%d, nodeId=%d", link.srcNodeSocketName.c_str(), link.srcNodeSocketIndex, link.srcNodeId);
				if (output == nullptr)
					logError("output node socket doesn't exist. name=%s, index=%d, nodeId=%d", link.dstNodeSocketName.c_str(), link.dstNodeSocketIndex, link.dstNodeId);
			}
			else
			{
				connectVfxSockets(srcNode, link.srcNodeSocketIndex, input, dstNode, link.dstNodeSocketIndex, output, link.params, true);
			}
		}
	}
	
	for (auto & nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		auto typeDefintion = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefintion == nullptr)
			continue;
		
		auto vfxNodeItr = vfxGraph->nodes.find(node.id);
		
		if (vfxNodeItr == vfxGraph->nodes.end())
			continue;
		
		VfxNodeBase * vfxNode = vfxNodeItr->second;
		
		auto & vfxNodeInputs = vfxNode->inputs;
		
		for (auto & inputValueItr : node.inputValues)
		{
			const std::string & inputName = inputValueItr.first;
			const std::string & inputValue = inputValueItr.second;
			
			bool found = false;
			
			for (size_t i = 0; i < typeDefintion->inputSockets.size(); ++i)
			{
				if (typeDefintion->inputSockets[i].name == inputName)
				{
					if (i < vfxNodeInputs.size())
					{
						found = true;
						
						if (vfxNodeInputs[i].isConnected() == false)
						{
							vfxGraph->connectToInputLiteral(vfxNodeInputs[i], inputValue);
						}
					}
				}
			}
			
			if (found == false)
			{
				logDebug("adding dynamic input socket value. socketName=%s, socketValue=%s", inputName.c_str(), inputValue.c_str());
				
				VfxDynamicInputSocketValue inputSocketValue;
				inputSocketValue.nodeId = node.id;
				inputSocketValue.socketName = inputName;
				inputSocketValue.value = inputValue;
				
				vfxGraph->dynamicData->inputSocketValues.push_back(inputSocketValue);
			}
		}
	}
	
	g_currentVfxGraph = vfxGraph;
	
	for (auto & vfxNodeItr : vfxGraph->nodes)
	{
		auto nodeId = vfxNodeItr.first;
		auto nodeItr = graph.nodes.find(nodeId);
		auto & node = nodeItr->second;
		auto vfxNode = vfxNodeItr.second;
		
		vfxNode->init(node);
	}
	
	g_currentVfxGraph = nullptr;
	
	return vfxGraph;
}

//

void connectVfxSockets(VfxNodeBase * srcNode, const int srcNodeSocketIndex, VfxPlug * srcSocket, VfxNodeBase * dstNode, const int dstNodeSocketIndex, VfxPlug * dstSocket, const std::map<std::string, std::string> & linkParams, const bool addPredep)
{
	srcSocket->connectTo(*dstSocket);

	// apply optional remapping parameters

	if (linkParams.empty() == false &&
		srcSocket->type == kVfxPlugType_Float &&
		dstSocket->type == kVfxPlugType_Float)
	{
		auto inMinItr = linkParams.find("in.min");
		auto inMaxItr = linkParams.find("in.max");
		auto outMinItr = linkParams.find("out.min");
		auto outMaxItr = linkParams.find("out.max");
		
		const bool hasRemap =
			inMinItr != linkParams.end() ||
			inMaxItr != linkParams.end() ||
			outMinItr != linkParams.end() ||
			outMaxItr != linkParams.end();
		
		if (hasRemap)
		{
			const float inMin = inMinItr == linkParams.end() ? 0.f : Parse::Float(inMinItr->second);
			const float inMax = inMaxItr == linkParams.end() ? 1.f : Parse::Float(inMaxItr->second);
			const float outMin = outMinItr == linkParams.end() ? 0.f : Parse::Float(outMinItr->second);
			const float outMax = outMaxItr == linkParams.end() ? 1.f : Parse::Float(outMaxItr->second);
			
			srcSocket->setMap(dstSocket->mem, inMin, inMax, outMin, outMax);
		}
	}
	
	if (addPredep)
	{
		// note : this may add the same node multiple times to the list of predeps. note that this
		//        is ok as nodes will be traversed once through the travel id + it works nicely
		//        with the live connection as we can just remove the predep and still have one or
		//        references to the predep if the predep was referenced more than once
		srcNode->predeps.push_back(dstNode);
	}

	// if this is a trigger, add a trigger target to dstNode
	if (dstSocket->type == kVfxPlugType_Trigger)
	{
		VfxNodeBase::TriggerTarget triggerTarget;
		triggerTarget.srcNode = srcNode;
		triggerTarget.srcSocketIndex = srcNodeSocketIndex;
		triggerTarget.dstSocketIndex = dstNodeSocketIndex;
		
		dstNode->triggerTargets.push_back(triggerTarget);
	}
}
