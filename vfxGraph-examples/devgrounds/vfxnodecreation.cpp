#include "audioGraphManager.h"
#include "Debugging.h"
#include "framework.h"
#include "soundmix.h"
#include "Timer.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"

extern AudioVoiceManager * g_vfxAudioVoiceMgr;
extern AudioGraphManager * g_vfxAudioGraphMgr;

static VfxPlugType stringToVfxPlugType(const std::string & typeName)
{
	VfxPlugType type = kVfxPlugType_None;

	if (typeName == "bool")
		type = kVfxPlugType_Bool;
	else if (typeName == "int")
		type = kVfxPlugType_Int;
	else if (typeName == "float")
		type = kVfxPlugType_Float;
	else if (typeName == "string")
		type = kVfxPlugType_String;
	else if (typeName == "channel")
		type = kVfxPlugType_Channel;
	else if (typeName == "color")
		type = kVfxPlugType_Color;
	else if (typeName == "image")
		type = kVfxPlugType_Image;
	else if (typeName == "image_cpu")
		type = kVfxPlugType_ImageCpu;
	else if (typeName == "draw")
		type = kVfxPlugType_Draw;
	else if (typeName == "trigger")
		type = kVfxPlugType_Trigger;
	
	return type;
}

void testVfxNodeCreation()
{
	VfxGraph vfxGraph;
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = &vfxGraph;
	
	AudioVoiceManagerBasic audioVoiceMgr;
	g_vfxAudioVoiceMgr = &audioVoiceMgr;
	
	AudioGraphManager_Basic audioGraphMgr(false);
	g_vfxAudioGraphMgr = &audioGraphMgr;
	
	for (VfxNodeTypeRegistration * registration = g_vfxNodeTypeRegistrationList; registration != nullptr; registration = registration->next)
	{
		const int64_t t1 = g_TimerRT.TimeUS_get();
		
		const int numIterations = 10;
		
		for (int n = 0; n < numIterations; ++n)
		{
			VfxNodeBase * vfxNode = registration->create();
			
			GraphNode node;
			
			vfxNode->initSelf(node);
			
			vfxNode->init(node);
			
			delete vfxNode;
			vfxNode = nullptr;
		}
		
		const int64_t t2 = g_TimerRT.TimeUS_get();
		
		logDebug("node create/destroy took %dus for %d iterations. nodeType=%s", t2 - t1, numIterations, registration->typeName.c_str()); (void)t1; (void)t2;
		
		//
		
		VfxNodeBase * vfxNode = registration->create();
		
		GraphNode node;

		vfxNode->initSelf(node);

		vfxNode->init(node);
		
		// check if vfx node is created properly
		
		Assert(vfxNode->inputs.size() == registration->inputs.size());
		const int numInputs = std::max(vfxNode->inputs.size(), registration->inputs.size());
		for (int i = 0; i < numInputs; ++i)
		{
			if (i >= vfxNode->inputs.size())
			{
				logError("input in registration doesn't exist in vfx node: nodeTypeName=%s, index=%d, name=%s", registration->typeName.c_str(), i, registration->inputs[i].name.c_str());
			}
			else if (i >= registration->inputs.size())
			{
				logError("input in vfx node doesn't exist in registration: nodeTypeName=%s, index=%d, type=%d", registration->typeName.c_str(), i, vfxNode->inputs[i].type);
			}
			else
			{
				auto & r = registration->inputs[i];
				
				const VfxPlugType type = stringToVfxPlugType(r.typeName);
				
				if (type == kVfxPlugType_None)
					logError("unknown type name in registration: nodeTypeName=%s, index=%d, typeName=%s", registration->typeName.c_str(), i, r.typeName.c_str());
				else if (type != vfxNode->inputs[i].type)
					logError("different types in registration vs vfx node. nodeTypeName=%s, index=%d, typeName=%s", registration->typeName.c_str(), i, r.typeName.c_str());
				
				if (type == kVfxPlugType_Trigger && r.name.size() > 0 && r.name.back() != '!')
					logError("name for input trigger doesn't end with '!'. nodeTypeName=%s, index=%d, typeName=%s", registration->typeName.c_str(), i, r.typeName.c_str());
			}
		}
		
		Assert(vfxNode->outputs.size() == registration->outputs.size());
		const int numOutputs = std::max(vfxNode->outputs.size(), registration->outputs.size());
		for (int i = 0; i < numOutputs; ++i)
		{
			if (i >= vfxNode->outputs.size())
			{
				logError("output in registration doesn't exist in vfx node: nodeTypeName=%s, index=%d, name=%s", registration->typeName.c_str(), i, registration->outputs[i].name.c_str());
			}
			else if (i >= registration->outputs.size())
			{
				logError("output in vfx node doesn't exist in registration: nodeTypeName=%s, index=%d, type=%d", registration->typeName.c_str(), i, vfxNode->outputs[i].type);
			}
			else
			{
				auto & r = registration->outputs[i];
				
				const VfxPlugType type = stringToVfxPlugType(r.typeName);
				
				if (type == kVfxPlugType_None)
					logError("unknown type name in registration: nodeTypeName=%s, index=%d, typeName=%s", registration->typeName.c_str(), i, r.typeName.c_str());
				else if (type != vfxNode->outputs[i].type)
					logError("different types in registration vs vfx node. nodeTypeName=%s, index=%d, typeName=%s", registration->typeName.c_str(), i, r.typeName.c_str());
				
				if (type == kVfxPlugType_Trigger && r.name.size() > 0 && r.name.back() != '!')
					logError("name for output trigger doesn't end with '!'. nodeTypeName=%s, index=%d, typeName=%s", registration->typeName.c_str(), i, r.typeName.c_str());
			}
		}
		
		delete vfxNode;
		vfxNode = nullptr;
	}
	
	g_currentVfxGraph = nullptr;
}
