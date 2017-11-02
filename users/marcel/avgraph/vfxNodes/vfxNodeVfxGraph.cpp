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

#include "tinyxml2.h"
#include "vfxGraph.h"
#include "vfxNodeVfxGraph.h"

using namespace tinyxml2;

VFX_NODE_TYPE(vfxGraph, VfxNodeVfxGraph)
{
	typeName = "vfxGraph";
	
	in("file", "string");
	out("image", "image");
}

VfxNodeVfxGraph::VfxNodeVfxGraph()
	: VfxNodeBase()
	, typeDefinitionLibrary(nullptr)
	, graph(nullptr)
	, vfxGraph(nullptr)
	, currentFilename()
	, imageOutput(nullptr)
{
	imageOutput = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_Image, imageOutput);
}

VfxNodeVfxGraph::~VfxNodeVfxGraph()
{
	close();
	
	delete imageOutput;
	imageOutput = nullptr;
}

void VfxNodeVfxGraph::open(const char * filename)
{
	close();
	
	//
	
	bool success = false;
	
	XMLDocument d;
	
	if (d.LoadFile(filename) == XML_SUCCESS)
	{
		graph = new Graph();
		
		typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
		
		if (graph->loadXml(d.RootElement(), typeDefinitionLibrary))
		{
			vfxGraph = constructVfxGraph(*graph, typeDefinitionLibrary);
			
			if (vfxGraph != nullptr)
			{
				success = true;
			}
		}
	}
	
	if (success == false)
	{
		close();
	}
}

void VfxNodeVfxGraph::close()
{
	delete vfxGraph;
	vfxGraph = nullptr;
	
	delete graph;
	graph = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
	
	//
	
	imageOutput->texture = 0;
}

void VfxNodeVfxGraph::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeVfxGraph);
	
	if (isPassthrough)
	{
		close();
		return;
	}
	
	const char * filename = getInputString(kInput_Filename, "");
	
	if (filename != currentFilename)
	{
		currentFilename = filename;
		
		open(filename);
	}
	
	if (vfxGraph != nullptr)
	{
		auto restore = g_currentVfxGraph;
		g_currentVfxGraph = nullptr;
		{
			vfxGraph->tick(dt);
		}
		g_currentVfxGraph = restore;
	}
}

void VfxNodeVfxGraph::draw() const
{
	vfxCpuTimingBlock(VfxNodeVfxGraph);
	
	if (isPassthrough || vfxGraph == nullptr)
	{
		return;
	}
	
	auto restore = g_currentVfxGraph;
	g_currentVfxGraph = nullptr;
	{
		imageOutput->texture = vfxGraph->traverseDraw();
	}
	g_currentVfxGraph = restore;
}

void VfxNodeVfxGraph::init(const GraphNode & node)
{
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (filename != nullptr)
	{
		open(filename);
	}
}
