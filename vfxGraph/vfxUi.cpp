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

#include "StringEx.h"
#include "ui.h"
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
