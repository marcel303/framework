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

#include "oscEndpointMgr.h"
#include "vfxNodeOscReceive.h"
#include "vfxTypes.h"

//

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
		
		if (g_doActions && isLearning)
		{
			OscFirstReceivedPathLearner firstReceivedPath;
			
			for (auto & receiver : g_oscEndpointMgr.receivers)
				receiver.receiver.pollMessages(&firstReceivedPath);
			
			if (!firstReceivedPath.path.empty())
			{
				path->path = firstReceivedPath.path;
				
				isLearning = false;
				
				uiState->reset();
			}
		}
		
		if (path != nullptr)
		{
			doTextBox(path->path, "path", dt);
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
				isLearning = true;
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

VFX_NODE_TYPE(VfxNodeOscReceive)
{
	typeName = "osc.receive";
	
	resourceTypeName = "osc.path";
	
	createResourceEditor = []() -> GraphEdit_ResourceEditorBase*
	{
		return new ResourceEditor_OscPath();
	};
	
	in("endpoint", "string");
	in("path", "string");
	out("value", "float");
	out("receive!", "trigger");
}

VfxNodeOscReceive::VfxNodeOscReceive()
	: VfxNodeBase()
	, oscPath(nullptr)
	, valueOutput(0.f)
	, history()
	, numReceives(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_EndpointName, kVfxPlugType_String);
	addInput(kInput_Path, kVfxPlugType_String);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Receive, kVfxPlugType_Trigger, nullptr);
}

VfxNodeOscReceive::~VfxNodeOscReceive()
{
	freeVfxNodeResource(oscPath);
}

const char * VfxNodeOscReceive::getPath() const
{
	const char * path = getInputString(kInput_Path, nullptr);
	
	if (path != nullptr)
		return path;
	
	return oscPath->path.c_str();
}

void VfxNodeOscReceive::init(const GraphNode & node)
{
	createVfxNodeResource<VfxOscPath>(node, "osc.path", "editorData", oscPath);
}

void VfxNodeOscReceive::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeOscReceive);
	
	if (isPassthrough)
		return;
	
	const char * endpointName = getInputString(kInput_EndpointName, "");
	
	auto receiver = g_oscEndpointMgr.findReceiver(endpointName);
	
	if (receiver != nullptr)
	{
		receiver->pollMessages(this);
	}
}

void VfxNodeOscReceive::getDescription(VfxNodeDescription & d)
{
	const char * path = getPath();
	
	d.add("path: %s", path);
	d.add("received values (%d total):", numReceives);
	if (history.empty())
		d.add("(none)");
	for (auto & h : history)
		d.add("%.6f", h.value);
}

void VfxNodeOscReceive::handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
{
	const char * path = getPath();
	
	if (strcmp(path, m.AddressPattern()) == 0)
	{
		for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
		{
			auto & a = *i;
			
			if (a.IsFloat())
			{
				valueOutput = a.AsFloat();
				
				trigger(kOutput_Receive);
				
				//

				HistoryItem historyItem;
				historyItem.value = valueOutput;
				history.push_front(historyItem);
				
				while (history.size() > kMaxHistory)
					history.pop_back();
				
				numReceives++;
			}
		}
	}
}
