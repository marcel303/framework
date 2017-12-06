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
#include "testBase.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodeBase.h"

#include "audioGraph.h"

void testVfxGraph()
{
	setAbout("This example shows Vfx Graph in action!");
	
	// fixme !
	fillPcmDataCache("../4dworld/testsounds", true, true);
	
	VfxGraph * vfxGraph = nullptr;
	
	RealTimeConnection rtc(vfxGraph);
	
	GraphEdit_TypeDefinitionLibrary tdl;
	createVfxTypeDefinitionLibrary(tdl, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
	
	GraphEdit graphEdit(&tdl, &rtc);
	
	graphEdit.load("testRibbon4.xml");
	
	do
	{
		framework.process();
		
		// graph edit may change the graph, so we tick it first
		graphEdit.tick(framework.timeStep, false);
		
		// update the vfx graph!
		if (rtc.vfxGraph != nullptr)
			rtc.vfxGraph->tick(GFX_SX, GFX_SY, framework.timeStep);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the vfx graph. this may update image outputs, so tick it before the visualizers
			if (rtc.vfxGraph != nullptr)
				rtc.vfxGraph->draw(GFX_SX, GFX_SY);
			
			// update the visualizers before we draw the editor
			graphEdit.tickVisualizers(framework.timeStep);
			
			// draw the editor!
			graphEdit.draw();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
