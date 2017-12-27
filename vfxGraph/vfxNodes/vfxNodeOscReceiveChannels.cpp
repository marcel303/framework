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
#include "vfxNodeOscReceiveChannels.h"
#include "vfxTypes.h"

//

#include "StringEx.h"
#include "tinyxml2.h"
#include "vfxGraph.h"
#include "../libparticle/ui.h" // todo : remove

struct ResourceEditor_OscPathList : GraphEdit_ResourceEditorBase
{
	UiState * uiState;
	VfxOscPathList * pathList;
	int learningIndex;
	
	ResourceEditor_OscPathList()
		: uiState(nullptr)
		, pathList(nullptr)
		, learningIndex(-1)
	{
		uiState = new UiState();
		uiState->sx = 400; // todo : look at GFX_SX
		uiState->textBoxTextOffset = 40;
	}
	
	~ResourceEditor_OscPathList()
	{
		freeVfxNodeResource(pathList);
		Assert(pathList == nullptr);
		
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
		pushMenu("osc.pathList");
		
		if (g_doActions && learningIndex != -1)
		{
			OscFirstReceivedPathLearner firstReceivedPath;
			
			for (auto & receiver : g_oscEndpointMgr.receivers)
				receiver.receiver.pollMessages(&firstReceivedPath);
			
			if (!firstReceivedPath.path.empty())
			{
				pathList->elems[learningIndex].path = firstReceivedPath.path;
				
				learningIndex = -1;
				
				uiState->reset();
			}
		}
		
		if (pathList != nullptr)
		{
			int removeIndex = -1;
			
			int index = 0;
			
			for (auto & elem : pathList->elems)
			{
				char name[32];
				sprintf_s(name, sizeof(name), "elem%d", index);
				
				pushMenu(name);
				{
					if (doButton(index == learningIndex ? "cancel" : "learn", 0.f, .15f, false))
					{
						if (index == learningIndex)
							learningIndex = -1;
						else
							learningIndex = index;
					}
					
					if (doButton("remove", 0.15f, .15f, false))
						removeIndex = index;
					
					doTextBox(elem.name, "name", .3f, .15f, false, dt);
					doTextBox(elem.path, "path", .45f, .55f, true, dt);
				}
				popMenu();
				
				index++;
			}
			
			if (removeIndex >= 0 && removeIndex < pathList->elems.size())
			{
				pathList->elems.erase(pathList->elems.begin() + removeIndex);
				
				uiState->reset();
				
				learningIndex = -1;
			}
		}
		
		if (doButton("add"))
		{
			pathList->elems.emplace_back();
			
			uiState->reset();
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
		const_cast<ResourceEditor_OscPathList*>(this)->doMenu(0.f);
	}
	
	void setResource(const GraphNode & node, const char * type, const char * name) override
	{
		Assert(pathList == nullptr);
		
		if (createVfxNodeResource<VfxOscPathList>(node, type, name, pathList))
		{
			//
		}
	}

	bool serializeResource(std::string & text) const override
	{
		if (pathList != nullptr)
		{
			tinyxml2::XMLPrinter p;
			p.OpenElement("value");
			{
				pathList->save(&p);
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

VFX_NODE_TYPE(VfxNodeOscReceiveChannels)
{
	typeName = "osc.receiveChannels";
	
	resourceTypeName = "osc.pathList";
	
	createResourceEditor = []() -> GraphEdit_ResourceEditorBase*
	{
		return new ResourceEditor_OscPathList();
	};
	
	in("endpoint", "string");
	out("channel", "channel");
	out("receive!", "trigger");
}

VfxNodeOscReceiveChannels::VfxNodeOscReceiveChannels()
	: VfxNodeBase()
	, oscPathList(nullptr)
	, channelData()
	, channelOutput()
	, history()
	, numReceives(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_EndpointName, kVfxPlugType_String);
	addOutput(kOutput_Receive, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

VfxNodeOscReceiveChannels::~VfxNodeOscReceiveChannels()
{
	freeVfxNodeResource(oscPathList);
}

void VfxNodeOscReceiveChannels::init(const GraphNode & node)
{
	createVfxNodeResource<VfxOscPathList>(node, "osc.pathList", "editorData", oscPathList);
}

void VfxNodeOscReceiveChannels::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeOscReceiveChannels);
	
	if (isPassthrough)
	{
		channelData.free();
		channelOutput.reset();
		setDynamicOutputs(nullptr, 0);
		return;
	}
	
	const char * endpointName = getInputString(kInput_EndpointName, "");
	
	// check if path list has changed. if it has, update dynamic outputs and channel output
	
	bool oscPathListHasChanged = false;
	
	int numNamedElems = 0;
	
	for (auto & elem : oscPathList->elems)
		if (elem.name.empty() == false)
			numNamedElems++;
	
	if (numNamedElems != dynamicOutputs.size())
		oscPathListHasChanged = true;
	else
	{
		int elemIndex = 0;
		
		for (auto & elem : oscPathList->elems)
		{
			if (elem.name.empty())
				continue;
			
			if (dynamicOutputs[elemIndex].name != elem.name)
				oscPathListHasChanged = true;
			
			elemIndex++;
		}
	}
	
	const int numElems = oscPathList->elems.size();
	
	if (numElems != channelData.size)
	{
		if (numElems == 0)
		{
			channelData.free();
			channelOutput.reset();
		}
		else
		{
			channelData.alloc(numElems);
			channelOutput.setData(channelData.data, false, numElems);
			
			for (int i = 0; i < numElems; ++i)
				channelData.data[i] = 0.f;
		}
	}
	
	if (oscPathListHasChanged)
	{
		if (numNamedElems == 0)
		{
			setDynamicOutputs(nullptr, 0);
		}
		else
		{
			DynamicOutput outputs[numNamedElems];
			
			int elemIndex = 0;
			
			for (int i = 0; i < numElems; ++i)
			{
				channelData.data[i] = 0.f;
				
				auto & elem = oscPathList->elems[i];
				
				if (elem.name.empty())
					continue;
				
				outputs[elemIndex].name = elem.name;
				outputs[elemIndex].type = kVfxPlugType_Float;
				outputs[elemIndex].mem = &channelData.data[i];
				
				elemIndex++;
			}
			
			setDynamicOutputs(outputs, numNamedElems);
		}
	}
	
	auto receiver = g_oscEndpointMgr.findReceiver(endpointName);
	
	if (receiver != nullptr)
	{
		receiver->pollMessages(this);
	}
}

void VfxNodeOscReceiveChannels::getDescription(VfxNodeDescription & d)
{
	d.add("pathList:");
	if (oscPathList->elems.empty())
		d.add("(empty)");
	else
	{
		for (auto & elem : oscPathList->elems)
			d.add("%s: %s", elem.name.c_str(), elem.path.c_str());
	}
	d.newline();
	
	d.add("received values (%d total):", numReceives);
	if (history.empty())
		d.add("(none)");
	for (auto & h : history)
		d.add("%s: %.6f", h.path.c_str(), h.value);
}

void VfxNodeOscReceiveChannels::handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
{
	bool hasReceived = false;
	
	int index = 0;
	
	for (auto & elem : oscPathList->elems)
	{
		if (elem.path == m.AddressPattern())
		{
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				auto & a = *i;
				
				if (a.IsFloat())
				{
					channelData.data[index] = a.AsFloat();
					
					hasReceived = true;
					
					//
					
					HistoryItem historyItem;
					historyItem.path = elem.path;
					historyItem.value = channelData.data[index];
					history.push_front(historyItem);
					
					while (history.size() > kMaxHistory)
						history.pop_back();
					
					numReceives++;
				}
			}
		}
		
		index++;
	}
	
	if (hasReceived)
	{
		trigger(kOutput_Receive);
	}
}
