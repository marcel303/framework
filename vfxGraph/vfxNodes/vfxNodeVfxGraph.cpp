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

#include "tinyxml2.h"
#include "vfxGraph.h"
#include "vfxNodeOutput.h"
#include "vfxNodeVfxGraph.h"
#include <algorithm>

using namespace tinyxml2;

extern VfxGraph * g_currentVfxGraph;

VFX_NODE_TYPE(VfxNodeVfxGraph)
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
	
	clearEditorIssue();
	
	XMLDocument d;
	
	if (d.LoadFile(filename) != XML_SUCCESS)
	{
		setEditorIssue("failed to open file");
	}
	else
	{
		graph = new Graph();
		
		typeDefinitionLibrary = createVfxTypeDefinitionLibrary();
		
		if (graph->loadXml(d.RootElement(), typeDefinitionLibrary) == false)
		{
			setEditorIssue("failed to parse XML");
		}
		else
		{
			auto restore = g_currentVfxGraph;
			g_currentVfxGraph = nullptr;
			{
				vfxGraph = constructVfxGraph(*graph, typeDefinitionLibrary);
			}
			g_currentVfxGraph = restore;
			
			if (vfxGraph == nullptr)
			{
				setEditorIssue("failed to create vfx graph");
			}
			else
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
	setDynamicOutputs(nullptr, 0);
	
	auto restore = g_currentVfxGraph;
	g_currentVfxGraph = nullptr;
	{
		delete vfxGraph;
		vfxGraph = nullptr;
	}
	g_currentVfxGraph = restore;
	
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
		currentFilename.clear();
		
		close();
		
		return;
	}
	
	// close the current file when its file contents have changed. this will cause a reopen later on
	
	if (framework.fileHasChanged(currentFilename.c_str()))
	{
		currentFilename.clear();
		
		close();
	}
	
	// check if we should open or close the file
	
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (filename == nullptr)
	{
		currentFilename.clear();
		
		close();
	}
	else if (filename != currentFilename)
	{
		currentFilename = filename;
		
		open(filename);
	}
	
	// process the graph (if opened)
	
	if (vfxGraph != nullptr)
	{
		const int sx = g_currentVfxGraph->sx;
		const int sy = g_currentVfxGraph->sy;
		
		auto restore = g_currentVfxGraph;
		g_currentVfxGraph = nullptr;
		{
			vfxGraph->tick(sx, sy, dt);
		}
		g_currentVfxGraph = restore;
		
		// update dynamic outputs
		
		std::vector<DynamicOutput> outputs;
		outputs.reserve(vfxGraph->outputNodes.size());
	
		for (auto outputNode : vfxGraph->outputNodes)
		{
			DynamicOutput output;
			output.name = outputNode->name;
			output.type = kVfxPlugType_Float;
			output.mem = &outputNode->value;
			
			outputs.push_back(output);
		}
		
		std::sort(outputs.begin(), outputs.end(), [](DynamicOutput & o1, DynamicOutput & o2) { return o1.name < o2.name; });
	
		setDynamicOutputs(outputs.data(), outputs.size());
	}
}

void VfxNodeVfxGraph::draw() const
{
	vfxCpuTimingBlock(VfxNodeVfxGraph);
	
	if (isPassthrough || vfxGraph == nullptr)
	{
		return;
	}
	
	auto restoreGraph = g_currentVfxGraph;
	auto restoreSurface = g_currentVfxSurface;
	g_currentVfxGraph = nullptr;
	g_currentVfxSurface = nullptr;
	{
		imageOutput->texture = vfxGraph->traverseDraw();
	}
	g_currentVfxGraph = restoreGraph;
	g_currentVfxSurface = restoreSurface;
}

void VfxNodeVfxGraph::init(const GraphNode & node)
{
	if (isPassthrough)
		return;
	
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (filename != nullptr)
	{
		currentFilename = filename;
		
		open(filename);
	}
}
