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

#include "Debugging.h"
#include "editor_oscPath.h"
#include "tinyxml2.h"
#include "ui.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxNodes/oscReceiver.h"
#include "vfxResource.h"
#include "vfxTypes.h"

extern int GRAPHEDIT_SX;
extern int GRAPHEDIT_SY;

//

ResourceEditor_OscPath::ResourceEditor_OscPath()
	: GraphEdit_ResourceEditorBase(GRAPHEDIT_SX*2/3, 100)
	, uiState(nullptr)
	, path(nullptr)
	, isLearning(false)
{
	uiState = new UiState();
}

ResourceEditor_OscPath::~ResourceEditor_OscPath()
{
	freeVfxNodeResource(path);
	Assert(path == nullptr);
	
	delete uiState;
	uiState = nullptr;
}

void ResourceEditor_OscPath::doMenu(const bool doTick, const bool doDraw, const float dt)
{
	uiState->sx = sx;
	uiState->x = x;
	uiState->y = y;
	
	makeActive(uiState, doTick, doDraw);
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

bool ResourceEditor_OscPath::tick(const float dt, const bool inputIsCaptured)
{
	doMenu(true, false, dt);
	
	return uiState->activeElem != nullptr;
}

void ResourceEditor_OscPath::draw() const
{
	const_cast<ResourceEditor_OscPath*>(this)->doMenu(false, true, 0.f);
}

void ResourceEditor_OscPath::setResource(const GraphNode & node, const char * type, const char * name)
{
	Assert(path == nullptr);
	
	if (createVfxNodeResource<VfxOscPath>(node, type, name, path))
	{
		//
	}
}

bool ResourceEditor_OscPath::serializeResource(std::string & text) const
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
