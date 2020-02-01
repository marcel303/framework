/*
	Copyright (C) 2020 Marcel Smit
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

#include "graphEdit.h"
#include "StringEx.h"
#include "ui.h"
#include "vfxGraph.h"
#include "vfxGraphManager.h"
#include "vfxUi.h"

bool doVfxGraphSelect(VfxGraphManager_RTE & vfxGraphMgr)
{
	bool result = false;
	
	for (auto & fileItr : vfxGraphMgr.files)
	{
		auto & filename = fileItr.first;
		
		if (doButton(filename.c_str()))
		{
			vfxGraphMgr.selectFile(filename.c_str());
			
			result = true;
		}
	}
	
	return result;
}

bool doVfxGraphInstanceSelect(VfxGraphManager_RTE & vfxGraphMgr, std::string & activeInstanceName)
{
	bool result = true;
	
	for (auto & fileItr : vfxGraphMgr.files)
	{
		auto & filename = fileItr.first;
		auto file = fileItr.second;
		
		for (auto instance : file->instances)
		{
			std::string name = String::FormatC("%s: %p", filename.c_str(), instance->vfxGraph);
			
			if (doButton(name.c_str()))
			{
				if (name != activeInstanceName)
				{
					activeInstanceName = name;
					
					auto oldSelectedFile = vfxGraphMgr.selectedFile;
					
					vfxGraphMgr.selectInstance(instance);
					
					if (oldSelectedFile != nullptr && vfxGraphMgr.selectedFile != nullptr)
					{
						vfxGraphMgr.selectedFile->graphEdit->dragAndZoom.focusX = oldSelectedFile->graphEdit->dragAndZoom.focusX;
						vfxGraphMgr.selectedFile->graphEdit->dragAndZoom.focusY = oldSelectedFile->graphEdit->dragAndZoom.focusY;
						vfxGraphMgr.selectedFile->graphEdit->dragAndZoom.zoom = oldSelectedFile->graphEdit->dragAndZoom.zoom;
					}
					
					result = true;
				}
			}
		}
	}
	
	return result;
}

void doVfxMemEditor(VfxGraph & vfxGraph, const float dt)
{
	doMemEditor(vfxGraph.memory, dt);
}

//

void doMemEditor(MemoryComponent & memory, const float dt)
{
	auto oldOffset = g_uiState->textBoxTextOffset;
	
	g_uiState->textBoxTextOffset = g_uiState->sx / 3;
	
	for (auto & mem_itr : memory.mems)
	{
		auto & name = mem_itr.first;
		auto & mem = mem_itr.second;
		
		std::string value = mem.value;
		
		doTextBox(value, name.c_str(), dt);
		
		if (value != mem.value)
			memory.setMems(name.c_str(), value.c_str());
	}
	
	g_uiState->textBoxTextOffset = 12;

	for (auto & mem_itr : memory.memf)
	{
		auto & name = mem_itr.first;
		auto & mem = mem_itr.second;
		
		Vec4 value = mem.value;
		
		doLabel(name.c_str(), 0.f);
		
		doTextBox(value[0], "x", 0.f / 4.f, 1.f / 4.f, false, dt);
		doTextBox(value[1], "y", 1.f / 4.f, 1.f / 4.f, false, dt);
		doTextBox(value[2], "z", 2.f / 4.f, 1.f / 4.f, false, dt);
		doTextBox(value[3], "w", 3.f / 4.f, 1.f / 4.f, false, dt);
		
		if (value[0] != mem.value[0] ||
			value[1] != mem.value[1] ||
			value[2] != mem.value[2] ||
			value[3] != mem.value[3])
		{
			memory.setMemf(name.c_str(), value[0], value[1], value[2], value[3]);
		}
	}
	
	g_uiState->textBoxTextOffset = oldOffset;
}
