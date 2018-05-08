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

#pragma once

#include "graph.h"
#include <map>
#include <set>
#include <string>
#include <vector>

#if defined(DEBUG)
	#define VFX_GRAPH_ENABLE_TIMING 1 // times node creation, destruction, node registration lookup
#else
	#define VFX_GRAPH_ENABLE_TIMING 0
#endif

class Surface;

struct VfxDynamicData;
struct VfxDynamicInputSocketValue;
struct VfxDynamicLink;

struct VfxGraph;
struct VfxNodeBase;
struct VfxPlug;
struct VfxResourceBase;

struct VfxNodeDisplay;
struct VfxNodeOutput;

extern VfxGraph * g_currentVfxGraph;
extern Surface * g_currentVfxSurface;

struct VfxGraph
{
	struct ValueToFree
	{
		enum Type
		{
			kType_Unknown,
			kType_Bool,
			kType_Int,
			kType_Float,
			kType_Transform,
			kType_String,
			kType_Color,
			kType_Channel
		};
		
		Type type;
		void * mem;
		
		ValueToFree()
			: type(kType_Unknown)
			, mem(nullptr)
		{
		}
		
		ValueToFree(const Type _type, void * _mem)
			: type(_type)
			, mem(_mem)
		{
		}
	};
	
	std::map<GraphNodeId, VfxNodeBase*> nodes;
	std::set<GraphNodeId> nodesFailedToCreate;
	
	VfxDynamicData * dynamicData;
	
	std::set<GraphNodeId> displayNodeIds;
	std::set<VfxNodeOutput*> outputNodes;
	
	int currentTickTraversalId;
	mutable int nextDrawTraversalId;
	
	std::vector<ValueToFree> valuesToFree;
	
	double time;
	
	VfxGraph();
	~VfxGraph();
	
	void destroy();
	void connectToInputLiteral(VfxPlug & input, const std::string & inputValue);
	
	void tick(const int sx, const int sy, const float dt);
	void draw(const int sx, const int sy) const;
	int traverseDraw(const int sx, const int sy) const;
	
	VfxNodeDisplay * getMainDisplayNode() const;
};

//

struct VfxDynamicLink
{
	int linkId;
	
	int srcNodeId;
	std::string srcSocketName;
	int srcSocketIndex;
	
	int dstNodeId;
	std::string dstSocketName;
	int dstSocketIndex;
	
	std::map<std::string, std::string> params;
	
	VfxDynamicLink()
		: linkId(-1)
		, srcNodeId(-1)
		, srcSocketName()
		, srcSocketIndex(-1)
		, dstNodeId(-1)
		, dstSocketName()
		, dstSocketIndex(-1)
		, params()
	{
	}
	
	float floatParam(const char * name, const float defaultValue) const;
};

struct VfxDynamicInputSocketValue
{
	int nodeId;
	std::string socketName;
	std::string value;
	
	VfxDynamicInputSocketValue()
		: nodeId(-1)
		, socketName()
		, value()
	{
	}
};

struct VfxDynamicData
{
	std::vector<VfxDynamicLink> links;
	
	std::vector<VfxDynamicInputSocketValue> inputSocketValues;
};

//

void createVfxTypeDefinitionLibrary(GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary);

VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName);

VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);

void connectVfxSockets(VfxNodeBase * srcNode, const int srcNodeSocketIndex, VfxPlug * srcSocket, VfxNodeBase * dstNode, const int dstNodeSocketIndex, VfxPlug * dstSocket, const std::map<std::string, std::string> & linkParams, const bool addPredep);
