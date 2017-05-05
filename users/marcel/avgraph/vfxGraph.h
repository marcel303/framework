#pragma once

#include "graph.h"
#include <map>
#include <string>
#include <vector>

struct VfxNodeBase;
struct VfxPlug;

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
			kType_Color
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
	
	GraphNodeId displayNodeId;
	
	mutable int nextTickTraversalId;
	mutable int nextDrawTraversalId;
	
	Graph * graph; // todo : remove ?
	
	std::vector<ValueToFree> valuesToFree;
	
	VfxGraph();
	~VfxGraph();
	
	void destroy();
	void connectToInputLiteral(VfxPlug & input, const std::string & inputValue);
	
	void tick(const float dt);
	void draw() const;
};
