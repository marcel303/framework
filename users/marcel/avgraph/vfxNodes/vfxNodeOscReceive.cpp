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
#include "vfxNodeOscReceive.h"
#include "vfxTypes.h"

// todo : refactor OSC receive node
// - add OSC receiver resource. shared between VFX graph instances
// - add OSC receive path resource editor. query OSC receiver resource and iterate messages
// - OSC receiver should be created if no VFX graph is running in the background
//	- but how will it know the IP address ? should IP address and port be options of the OSC receiver resource editor ?
//	- perhaps OSC path editor should only work when VFX graph is active after all ..

#include "StringEx.h"
#include "tinyxml2.h"
#include "vfxGraph.h"
#include "../libparticle/ui.h" // todo : remove

struct ResourceEditor_OscPath : GraphEdit_ResourceEditorBase
{
	UiState * uiState;
	VfxOscPath * path;
	bool isLearning;
	
	ResourceEditor_OscPath()
		: uiState(nullptr)
		, path(nullptr)
		, isLearning(false)
	{
		uiState = new UiState();
		uiState->sx = 400; // todo : look at GFX_SX
	}
	
	~ResourceEditor_OscPath()
	{
		freeVfxNodeResource(path);
		Assert(path == nullptr);
		
		delete uiState;
		uiState = nullptr;
	}
	
	void getSize(int & sx, int & sy) const override
	{
		sx = uiState->sx;
		sy = 100;
	}

	void setPosition(const int x, const int y) override
	{
		uiState->x = x;
		uiState->y = y;
	}
	
	void doMenu(const float dt)
	{
		pushMenu("osc.path");
		
		if (path != nullptr)
		{
			std::string value = path->path;
			
			doTextBox(value, "path", dt);
			
			strcpy_s(path->path, sizeof(path->path), value.c_str());
		}
		
		if (isLearning)
		{
			doLabel("learning..", 0.f);
			
			if (doButton("cancel"))
				isLearning = false;
		}
		else
		{
			if (doButton("learn"))
			{
				isLearning = true;
			}
		}
		
		popMenu();
	}
	
	bool tick(const float dt, const bool inputIsCaptured) override
	{
		makeActive(uiState, true, false);
		doMenu(dt);
		
		return uiState->activeElem != nullptr;
	}

	void draw() const override
	{
		makeActive(uiState, false, true);
		const_cast<ResourceEditor_OscPath*>(this)->doMenu(0.f);
	}
	
	void setResource(const GraphNode & node, const char * type, const char * name) override
	{
		Assert(path == nullptr);
		
		if (createVfxNodeResource<VfxOscPath>(node, type, name, path))
		{
			//
		}
	}

	bool serializeResource(std::string & text) const override
	{
		if (path != nullptr)
		{
			tinyxml2::XMLPrinter p;
			p.OpenElement("value");
			{
				path->save(&p);
			}
			p.CloseElement();
			
			text = p.CStr();
			
			return true;
		}
		else
		{
			return false;
		}
	}
};

//

#include "oscReceiver.h"
#include <map>

struct VfxOscEndpointMgr : OscReceiveHandler
{
	struct Receiver
	{
		OscReceiver receiver;
		int refCount;
		
		Receiver()
			: receiver()
			, refCount(0)
		{
		}
	};
	
	std::list<Receiver> receivers;
	std::map<std::string, std::vector<float>> receivedValues;
	int lastTraversalId;
	
	VfxOscEndpointMgr()
		: receivers()
		, receivedValues()
		, lastTraversalId(-1)
	{
	}
	
	OscReceiver * alloc(const char * ipAddress, const int udpPort)
	{
		for (auto & r : receivers)
		{
			if (r.receiver.ipAddress == ipAddress && r.receiver.udpPort == udpPort)
			{
				r.refCount++;
				
				return &r.receiver;
			}
		}
		
		{
			Receiver r;
			
			receivers.push_back(r);
		}
		
		auto & r = receivers.back();
		
		r.receiver.init(ipAddress, udpPort);
		
		r.refCount++;
		
		return &r.receiver;
	}
	
	void free(OscReceiver *& receiver)
	{
		for (auto rItr = receivers.begin(); rItr != receivers.end(); ++rItr)
		{
			auto & r = *rItr;
			
			if (receiver == &r.receiver)
			{
				r.refCount--;
				
				if (r.refCount == 0)
				{
					r.receiver.shut();
					
					receivers.erase(rItr);
					
					break;
				}
			}
		}
	}
	
	void tick()
	{
		if (g_currentVfxGraph->nextTickTraversalId != lastTraversalId)
		{
			lastTraversalId = g_currentVfxGraph->nextTickTraversalId;
			
			receivedValues.clear();
			
			for (auto & r : receivers)
			{
				r.receiver.tick(this);
			}
		}
	}
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		const char * path = nullptr;
		float value = 0.f;
		
		try
		{
			path = m.AddressPattern();
			
			auto args = m.ArgumentStream();
			
			args >> value;
		}
		catch (std::exception & e)
		{
			logError("failed to handle OSC message: %s", e.what());
			
			return;
		}
		
		//
		
		auto & values = receivedValues[path];
		
		values.push_back(value);
	}
};

static VfxOscEndpointMgr s_vfxOscEndpointMgr;

//

VFX_NODE_TYPE(osc_endpoint, VfxNodeOscEndpoint)
{
	typeName = "osc.endpoint";
	
	in("ipAddress", "string");
	in("port", "int");
}

VfxNodeOscEndpoint::VfxNodeOscEndpoint()
	: VfxNodeBase()
	, oscReceiver(nullptr)
	, history()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_IpAddress, kVfxPlugType_String);
	addInput(kInput_Port, kVfxPlugType_Int);
}

VfxNodeOscEndpoint::~VfxNodeOscEndpoint()
{
	s_vfxOscEndpointMgr.free(oscReceiver);
}

void VfxNodeOscEndpoint::init(const GraphNode & node)
{
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	oscReceiver = s_vfxOscEndpointMgr.alloc(ipAddress, udpPort);
}

void VfxNodeOscEndpoint::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeOscReceive);
	
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	if (oscReceiver->isAddressChange(ipAddress, udpPort))
	{
		s_vfxOscEndpointMgr.free(oscReceiver);
		
		oscReceiver = s_vfxOscEndpointMgr.alloc(ipAddress, udpPort);
	}
}

void VfxNodeOscEndpoint::getDescription(VfxNodeDescription & d)
{
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	d.add("bind address: %s:%d", ipAddress, udpPort);
	d.newline();
	
	d.add("received messages:");
	if (history.empty())
		d.add("(none)");
	for (auto & h : history)
		d.add("%s", h.addressPattern.c_str());
}

//

VFX_NODE_TYPE(osc_receive, VfxNodeOscReceive)
{
	typeName = "osc.receive";
	
	resourceTypeName = "osc.path";
	
	createResourceEditor = []() -> GraphEdit_ResourceEditorBase*
	{
		return new ResourceEditor_OscPath();
	};
	
	in("ipAddress", "string");
	in("port", "int");
	out("receive!", "trigger");
}

VfxNodeOscReceive::VfxNodeOscReceive()
	: VfxNodeBase()
	, oscPath(nullptr)
	, valueOutput(0.f)
	, history()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_IpAddress, kVfxPlugType_String);
	addInput(kInput_Port, kVfxPlugType_Int);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Receive, kVfxPlugType_Trigger, nullptr);
}

VfxNodeOscReceive::~VfxNodeOscReceive()
{
	freeVfxNodeResource(oscPath);
}

void VfxNodeOscReceive::init(const GraphNode & node)
{
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	createVfxNodeResource<VfxOscPath>(node, "osc.path", "editorData", oscPath);
}

void VfxNodeOscReceive::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeOscReceive);
	
	s_vfxOscEndpointMgr.tick();
	
	auto i = s_vfxOscEndpointMgr.receivedValues.find(oscPath->path);
	
	if (i != s_vfxOscEndpointMgr.receivedValues.end())
	{
		auto & values = i->second;
		
		for (auto value : values)
		{
			valueOutput = value;
	
			trigger(kOutput_Receive);
			
			//

			HistoryItem historyItem;
			historyItem.value = value;
			history.push_front(historyItem);
			
			while (history.size() > kMaxHistory)
				history.pop_back();
		}
	}
}

void VfxNodeOscReceive::getDescription(VfxNodeDescription & d)
{
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	d.add("bind address: %s:%d", ipAddress, udpPort);
	d.newline();
	
	d.add("path: %s", oscPath->path);
	d.add("received values:");
	if (history.empty())
		d.add("(none)");
	for (auto & h : history)
		d.add("%.2f", h.value);
}

