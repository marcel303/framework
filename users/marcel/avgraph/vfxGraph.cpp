#include "framework.h"
#include "Parse.h"
#include "Timer.h"
#include "vfxGraph.h"
#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeDisplay.h"

extern const int GFX_SX;
extern const int GFX_SY;

//

VfxGraph * g_currentVfxGraph = nullptr;

//

VfxGraph::VfxGraph()
	: nodes()
	, displayNodeId(kGraphNodeIdInvalid)
	, nextTickOrder(0)
	, nextTickTraversalId(0)
	, nextDrawTraversalId(0)
	, graph(nullptr)
	, valuesToFree()
	, time(0.0)
{
}

VfxGraph::~VfxGraph()
{
	destroy();
}

void VfxGraph::destroy()
{
	graph = nullptr;
	
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
		case ValueToFree::kType_Transform:
			delete (VfxTransform*)i.mem;
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
	
	for (auto i : nodes)
	{
		VfxNodeBase * node = i.second;
		
		delete node;
		node = nullptr;
	}
	
	nodes.clear();
}

void VfxGraph::connectToInputLiteral(VfxPlug & input, const std::string & inputValue)
{
	if (input.type == kVfxPlugType_Bool)
	{
		bool * value = new bool();
		
		*value = Parse::Bool(inputValue);
		
		input.connectTo(value, kVfxPlugType_Bool);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Bool, value));
	}
	else if (input.type == kVfxPlugType_Int)
	{
		int * value = new int();
		
		*value = Parse::Int32(inputValue);
		
		input.connectTo(value, kVfxPlugType_Int);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Int, value));
	}
	else if (input.type == kVfxPlugType_Float)
	{
		float * value = new float();
		
		*value = Parse::Float(inputValue);
		
		input.connectTo(value, kVfxPlugType_Float);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Float, value));
	}
	else if (input.type == kVfxPlugType_Transform)
	{
		VfxTransform * value = new VfxTransform();
		
		// todo : parse inputValue
		
		input.connectTo(value, kVfxPlugType_Transform);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_Transform, value));
	}
	else if (input.type == kVfxPlugType_String)
	{
		std::string * value = new std::string();
		
		*value = inputValue;
		
		input.connectTo(value, kVfxPlugType_String);
		
		valuesToFree.push_back(VfxGraph::ValueToFree(VfxGraph::ValueToFree::kType_String, value));
	}
	else if (input.type == kVfxPlugType_Color)
	{
		Color * value = new Color();
		
		*value = Color::fromHex(inputValue.c_str());
		
		input.connectTo(value, kVfxPlugType_Color);
		
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
	
	nextTickOrder = 0;
	
	if (displayNodeId != kGraphNodeIdInvalid)
	{
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

	// todo : perhaps process unconnected nodes as islands, following predeps ?
	
	for (auto i : nodes)
	{
		VfxNodeBase * node = i.second;
		
		if (node->lastTickTraversalId != nextTickTraversalId)
		{
			node->lastTickTraversalId = nextTickTraversalId;
			node->tickOrder = -1;
			
			const uint64_t t1 = g_TimerRT.TimeUS_get();
			
			node->tick(dt);
			
			const uint64_t t2 = g_TimerRT.TimeUS_get();
			
			node->tickTimeAvg = (node->tickTimeAvg * 90 + (t2 - t1) * 10) / 100;
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
	vfxCpuTimingBlock(VfxGraph_Draw);
	vfxGpuTimingBlock(VfxGraph_Draw);
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = const_cast<VfxGraph*>(this);
	
	// start traversal at the display node and traverse to leafs following predeps and and back up the tree again to draw
	
	if (displayNodeId != kGraphNodeIdInvalid)
	{
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
				gxSetTexture(image->getTexture());
				pushBlend(BLEND_OPAQUE);
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				popBlend();
				gxSetTexture(0);
			}
		}
	}
	
	++nextDrawTraversalId;
	
	//
	
#if 0 // todo : remove this code ?
	struct SortedNode
	{
		GraphNodeId nodeId;
		const VfxNodeBase * vfxNode;
	};
	SortedNode * sortedNodes = (SortedNode*)alloca((sizeof(SortedNode) * nodes.size()));
	int numSortedNodes = 0;
	for (auto i : nodes)
	{
		if (i.second->tickOrder == -1)
			continue;
		sortedNodes[numSortedNodes].nodeId = i.first;
		sortedNodes[numSortedNodes].vfxNode = i.second;
		numSortedNodes++;
	}
	std::sort(sortedNodes, sortedNodes + numSortedNodes, [](auto & n1, auto & n2) { return n1.vfxNode->tickOrder < n2.vfxNode->tickOrder; });
	
	setFont("calibri.ttf");
	beginTextBatch();
	{
		int x = GFX_SX/2;
		int y = 10;
		
		for (int i = 0; i < numSortedNodes; ++i)
		{
			const GraphNodeId nodeId = sortedNodes[i].nodeId;
			const GraphNode * node = graph->tryGetNode(nodeId);
			const VfxNodeBase * vfxNode = sortedNodes[i].vfxNode;
			
			setColor(colorWhite);
			drawText(x, y, 12, 0, 0, "node %d / %s: tickOrder: %d, tick: %.2fms, draw %.2fms", nodeId, node ? node->getDisplayName().c_str() : "n/a", vfxNode->tickOrder, vfxNode->tickTimeAvg/1000.0, vfxNode->drawTimeAvg/1000.0);
			
			y += 18;
		}
	}
	endTextBatch();
#endif
	
	//
	
	g_currentVfxGraph = nullptr;
}
