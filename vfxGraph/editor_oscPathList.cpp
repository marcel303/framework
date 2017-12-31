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
#include "editor_oscPathList.h"
#include "StringEx.h" // sprintf_s
#include "tinyxml2.h"
#include "vfxGraph.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxNodes/oscReceiver.h"
#include "vfxTypes.h"
#include "../libparticle/ui.h" // todo : remove

//

extern int GRAPHEDIT_SX;
extern int GRAPHEDIT_SY;

//

ResourceEditor_OscPathList::ResourceEditor_OscPathList()
	: uiState(nullptr)
	, pathList(nullptr)
	, learningIndex(-1)
{
	uiState = new UiState();
	uiState->sx = GRAPHEDIT_SX*2/3;
	uiState->textBoxTextOffset = 40;
}

ResourceEditor_OscPathList::~ResourceEditor_OscPathList()
{
	freeVfxNodeResource(pathList);
	Assert(pathList == nullptr);
	
	delete uiState;
	uiState = nullptr;
}

void ResourceEditor_OscPathList::getSize(int & sx, int & sy) const
{
	sx = uiState->sx;
	sy = 100;
}

void ResourceEditor_OscPathList::setPosition(const int x, const int y)
{
	uiState->x = x;
	uiState->y = y;
}

void ResourceEditor_OscPathList::doMenu(const float dt)
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

bool ResourceEditor_OscPathList::tick(const float dt, const bool inputIsCaptured)
{
	makeActive(uiState, true, false);
	doMenu(dt);
	
	return uiState->activeElem != nullptr;
}

void ResourceEditor_OscPathList::draw() const
{
	makeActive(uiState, false, true);
	const_cast<ResourceEditor_OscPathList*>(this)->doMenu(0.f);
}

void ResourceEditor_OscPathList::setResource(const GraphNode & node, const char * type, const char * name)
{
	Assert(pathList == nullptr);
	
	if (createVfxNodeResource<VfxOscPathList>(node, type, name, pathList))
	{
		//
	}
}

bool ResourceEditor_OscPathList::serializeResource(std::string & text) const
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
