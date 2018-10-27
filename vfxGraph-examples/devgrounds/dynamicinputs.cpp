#include "vfxGraph.h"
#include "vfxNodeBase.h"

void testDynamicInputs()
{
	VfxGraph g;
	
	VfxNodeBase * node1 = new VfxNodeBase();
	VfxNodeBase * node2 = new VfxNodeBase();
	
	g.nodes[0] = node1;
	g.nodes[1] = node2;
	
	float values[32];
	for (int i = 0; i < 32; ++i)
		values[i] = i;
	
	node1->resizeSockets(1, 2);
	node2->resizeSockets(2, 1);
	
	node1->addInput(0, kVfxPlugType_Float);
	node1->addOutput(0, kVfxPlugType_Float, &values[0]);
	node1->addOutput(1, kVfxPlugType_Float, &values[1]);
	node2->addInput(0, kVfxPlugType_Float);
	node2->addInput(1, kVfxPlugType_Float);
	node2->addOutput(0, kVfxPlugType_Float, &values[16]);
	
	node1->tryGetInput(0)->connectTo(*node2->tryGetOutput(0));
	node2->tryGetInput(0)->connectTo(*node1->tryGetOutput(0));
	node2->tryGetInput(1)->connectTo(*node1->tryGetOutput(1));
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "a";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketIndex = 0;
		g.dynamicData->links.push_back(link);
	}
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "b";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketIndex = 0;
		g.dynamicData->links.push_back(link);
	}
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "b";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketName = "a";
		link.dstSocketIndex = -1;
		g.dynamicData->links.push_back(link);
	}
	
	g_currentVfxGraph = &g;
	{
		VfxNodeBase::DynamicInput inputs[2];
		inputs[0].name = "a";
		inputs[0].type = kVfxPlugType_Float;
		inputs[1].name = "b";
		inputs[1].type = kVfxPlugType_Float;
		
		node1->setDynamicInputs(inputs, 2);
		
		node1->setDynamicInputs(nullptr, 0);
		
		node1->setDynamicInputs(inputs, 2);
		
		//
		
		float value = 1.f;
		
		VfxNodeBase::DynamicOutput outputs[1];
		outputs[0].name = "a";
		outputs[0].type = kVfxPlugType_Float;
		outputs[0].mem = &value;
		
		node2->setDynamicOutputs(outputs, 1);
		
		node2->setDynamicOutputs(nullptr, 0);
		
		node2->setDynamicOutputs(outputs, 1);
	}
	g_currentVfxGraph = nullptr;
}
