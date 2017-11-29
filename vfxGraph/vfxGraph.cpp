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

#include "framework.h"
#include "Parse.h"
#include "vfxGraph.h"
#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxTypes.h"

#if VFX_GRAPH_ENABLE_TIMING
	#include "Timer.h"
#endif

extern const int GFX_SX;
extern const int GFX_SY;

//

VfxGraph * g_currentVfxGraph = nullptr;

Surface * g_currentVfxSurface = nullptr;

static Surface * s_dummySurface = nullptr; // todo : add explicit vfx graph init and shutdown ?

//

VfxGraph::VfxGraph()
	: nodes()
	, dynamicData(nullptr)
	, displayNodeIds()
	, nextTickTraversalId(0)
	, nextDrawTraversalId(0)
	, valuesToFree()
	, time(0.0)
{
	dynamicData = new VfxDynamicData();
	
	if (s_dummySurface == nullptr)
	{
		s_dummySurface = new Surface(1, 1, false);
	}
}

VfxGraph::~VfxGraph()
{
	destroy();
}

void VfxGraph::destroy()
{
	displayNodeIds.clear();
	
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
		default:
			Assert(false);
			break;
		}
	}
	
	valuesToFree.clear();
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = this;
	
	for (auto i : nodes)
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
	
	g_currentVfxGraph = nullptr;
	
	delete dynamicData;
	dynamicData = nullptr;
	
	nodes.clear();
}

void VfxGraph::connectToInputLiteral(VfxPlug & input, const std::string & inputValue)
{
	if (input.type == kVfxPlugType_Bool)
	{
		bool * value = new bool();
		
		*value = Parse::Bool(inputValue);
		
		input.connectTo(value, kVfxPlugType_Bool, true);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Bool, value));
	}
	else if (input.type == kVfxPlugType_Int)
	{
		int * value = new int();
		
		*value = Parse::Int32(inputValue);
		
		input.connectTo(value, kVfxPlugType_Int, true);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Int, value));
	}
	else if (input.type == kVfxPlugType_Float)
	{
		float * value = new float();
		
		*value = Parse::Float(inputValue);
		
		input.connectTo(value, kVfxPlugType_Float, true);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Float, value));
	}
	else if (input.type == kVfxPlugType_String)
	{
		std::string * value = new std::string();
		
		*value = inputValue;
		
		input.connectTo(value, kVfxPlugType_String, true);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_String, value));
	}
	else if (input.type == kVfxPlugType_Color)
	{
		Color * value = new Color();
		
		*value = Color::fromHex(inputValue.c_str());
		
		input.connectTo(value, kVfxPlugType_Color, true);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Color, value));
	}
	else
	{
		logWarning("cannot instantiate literal for non-supported type %d, value=%s", input.type, inputValue.c_str());
	}
}

void VfxGraph::tick(const float dt)
{
	vfxCpuTimingBlock(VfxGraph_Tick);
	vfxGpuTimingBlock(VfxGraph_Tick);
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = this;
	
	// use traversalId, start update at display node
	
	if (displayNodeIds.empty() == false)
	{
		auto displayNodeId = *displayNodeIds.begin();
		
		auto nodeItr = nodes.find(displayNodeId);
		Assert(nodeItr != nodes.end());
		if (nodeItr != nodes.end())
		{
			auto node = nodeItr->second;
			
			VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
			
			displayNode->traverseTick(nextTickTraversalId, dt);
		}
	}
	
	// process nodes that aren't connected to the display node
	
	for (auto i : nodes)
	{
		VfxNodeBase * node = i.second;
		
		if (node->lastTickTraversalId != nextTickTraversalId)
		{
			node->traverseTick(nextTickTraversalId, dt);
		}
	}
	
	++nextTickTraversalId;
	
	//
	
	time += dt;
	
	//
	
	g_currentVfxGraph = nullptr;
}

void VfxGraph::draw() const
{
	const GLuint texture = traverseDraw();
	
	if (texture != 0)
	{
		gxSetTexture(texture);
		pushBlend(BLEND_OPAQUE);
		setColor(colorWhite);
		drawRect(0, 0, GFX_SX, GFX_SY);
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
	
	if (displayNodeIds.empty() == false)
	{
		auto displayNodeId = *displayNodeIds.begin();
		
		auto nodeItr = nodes.find(displayNodeId);
		Assert(nodeItr != nodes.end());
		if (nodeItr != nodes.end())
		{
			auto node = nodeItr->second;
			
			VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
			
			displayNode->traverseDraw(nextDrawTraversalId);
			
			const VfxImageBase * image = displayNode->getImage();
			
			if (image != nullptr)
			{
				result = image->getTexture();
			}
		}
	}
	
#if 1 // todo : make this depend on whether the graph editor is visible or not ? or whether the node is referenced by the editor ?

	pushSurface(s_dummySurface);
	{
		// draw nodes that aren't connected to the display node
		
		Assert(g_currentVfxSurface == nullptr);
		g_currentVfxSurface = s_dummySurface;
		
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			if (node->lastDrawTraversalId != nextDrawTraversalId)
			{
				//if (any input or output referencedByRealTimeConnectionTick)
				
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

//

VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName, VfxGraph * vfxGraph)
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
	
	if (typeName == "draw.display")
	{
		Assert(vfxNode != nullptr);
		if (vfxNode != nullptr)
		{
			// fixme : move display node id handling out of here. remove nodeId and vfxGraph passed in to this function
			Assert(vfxGraph->displayNodeIds.count(nodeId) == 0);
			vfxGraph->displayNodeIds.insert(nodeId);
		}
	}
	
	return vfxNode;
}

//

VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	VfxGraph * vfxGraph = new VfxGraph();
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = vfxGraph;
	
	for (auto & nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		VfxNodeBase * vfxNode = createVfxNode(node.id, node.typeName, vfxGraph);
		
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
				VfxDynamicLink dlink;
				dlink.linkId = link.id;
				dlink.srcNodeId = link.srcNodeId;
				dlink.srcSocketIndex = link.srcNodeSocketIndex;
				dlink.srcSocketName = link.srcNodeSocketName;
				dlink.dstNodeId = link.dstNodeId;
				dlink.dstSocketIndex = link.dstNodeSocketIndex;
				dlink.dstSocketName = link.dstNodeSocketName;
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
				input->connectTo(*output);
				
				// apply optional remapping parameters
				
				if (link.params.empty() == false &&
					input->type == kVfxPlugType_Float &&
					output->type == kVfxPlugType_Float)
				{
					auto inMinItr = link.params.find("in.min");
					auto inMaxItr = link.params.find("in.max");
					auto outMinItr = link.params.find("out.min");
					auto outMaxItr = link.params.find("out.max");
					
					const bool hasRemap =
						inMinItr != link.params.end() ||
						inMaxItr != link.params.end() ||
						outMinItr != link.params.end() ||
						outMaxItr != link.params.end();
					
					if (hasRemap)
					{
						const float inMin = inMinItr == link.params.end() ? 0.f : Parse::Float(inMinItr->second);
						const float inMax = inMaxItr == link.params.end() ? 1.f : Parse::Float(inMaxItr->second);
						const float outMin = outMinItr == link.params.end() ? 0.f : Parse::Float(outMinItr->second);
						const float outMax = outMaxItr == link.params.end() ? 1.f : Parse::Float(outMaxItr->second);
						
						input->setMap(output->mem, inMin, inMax, outMin, outMax);
					}
				}
				
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
		
		auto vfxNodeItr = vfxGraph->nodes.find(node.id);
		
		if (vfxNodeItr == vfxGraph->nodes.end())
			continue;
		
		VfxNodeBase * vfxNode = vfxNodeItr->second;
		
		auto & vfxNodeInputs = vfxNode->inputs;
		
		for (auto & inputValueItr : node.editorInputValues)
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

#include "StringEx.h"
#include "tinyxml2.h"
#include "vfxTypes.h"

using namespace tinyxml2;

struct VfxResourcePath
{
	GraphNodeId nodeId;
	std::string type;
	std::string name;
	
	VfxResourcePath()
		: nodeId(kGraphNodeIdInvalid)
		, type()
		, name()
	{
	}
	
	bool operator<(const VfxResourcePath & other) const
	{
		if (nodeId != other.nodeId)
			return nodeId < other.nodeId;
		if (type != other.type)
			return type < other.type;
		if (name != other.name)
			return name < other.name;
		
		return false;
	}
	
	std::string toString() const
	{
		return String::FormatC("%s:%s/%d/%s", type.c_str(), "nodes", nodeId, name.c_str());
	}
};

struct VfxResourceElem
{
	void * resource;
	int refCount;
	
	VfxResourceElem()
		: resource(nullptr)
		, refCount(0)
	{
	}
};

static std::map<VfxResourcePath, VfxResourceElem> resourcesByPath;
static std::map<void*, VfxResourcePath> pathsByResource;

bool createVfxNodeResourceImpl(const GraphNode & node, const char * type, const char * name, void *& resource)
{
	VfxResourcePath path;
	path.nodeId = node.id;
	path.type = type;
	path.name = name;
	
	auto i = resourcesByPath.find(path);
	
	if (i != resourcesByPath.end())
	{
		logDebug("incremented refCount for resource %s", path.toString().c_str());
		
		auto & e = i->second;
		
		e.refCount++;
		
		resource = e.resource;
		
		return true;
	}
	else
	{
		const char * resourceData = node.getResource(type, name, nullptr);
		
		XMLDocument d;
		bool hasXml = false;
		
		if (resourceData != nullptr)
		{
			hasXml = d.Parse(resourceData) == XML_SUCCESS;
		}
		
		//
		
		resource = nullptr;
		
		if (strcmp(type, "timeline") == 0)
		{
			auto timeline = new VfxTimeline();
			
			if (hasXml)
			{
				timeline->load(d.RootElement());
			}
			
			resource = timeline;
		}
		else if (strcmp(type, "osc.path") == 0)
		{
			auto path = new VfxOscPath();
			
			if (hasXml)
			{
				path->load(d.RootElement());
			}
			
			resource = path;
		}
		else if (strcmp(type, "osc.pathList") == 0)
		{
			auto pathList = new VfxOscPathList();
			
			if (hasXml)
			{
				pathList->load(d.RootElement());
			}
			
			resource = pathList;
		}
		
		//
		
		Assert(resource != nullptr);
		if (resource == nullptr)
		{
			logError("failed to create resource %s", path.toString().c_str());
			
			return false;
		}
		else
		{
			logDebug("created resource %s", path.toString().c_str());
			
			VfxResourceElem e;
			e.resource = resource;
			e.refCount = 1;
			
			resourcesByPath[path] = e;
			pathsByResource[resource] = path;
			
			return true;
		}
	}
}

bool freeVfxNodeResourceImpl(void * resource)
{
	bool result = false;
	
	auto i = pathsByResource.find(resource);
	
	Assert(i != pathsByResource.end());
	if (i == pathsByResource.end())
	{
		logError("failed to find resource %p", resource);
	}
	else
	{
		auto & path = i->second;
		
		auto j = resourcesByPath.find(path);
		
		Assert(j != resourcesByPath.end());
		if (j == resourcesByPath.end())
		{
			logError("failed to find resource elem for resource %s", path.toString().c_str());
		}
		else
		{
			auto & e = j->second;
			
			e.refCount--;
			
			if (e.refCount == 0)
			{
				logDebug("refCount reached zero for resource %s. resource will be freed", path.toString().c_str());
				
				resourcesByPath.erase(j);
				pathsByResource.erase(i);
				
				result = true;
			}
			else
			{
				logDebug("decremented refCount for resource %s", path.toString().c_str());
			}
		}
	}
	
	return result;
}
