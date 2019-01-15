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
#include "graph.h"
#include "graphEdit_nodeResourceEditorWindow.h"
#include "ui.h"

GraphEdit_NodeResourceEditorWindow::GraphEdit_NodeResourceEditorWindow()
	: graphEdit(nullptr)
	, nodeId(kGraphNodeIdInvalid)
	, resourceTypeName()
	, window(nullptr)
	, resourceEditor(nullptr)
{
}

GraphEdit_NodeResourceEditorWindow::~GraphEdit_NodeResourceEditorWindow()
{
	shut();
}

bool GraphEdit_NodeResourceEditorWindow::init(GraphEdit * in_graphEdit, const GraphNodeId in_nodeId)
{
	shut();

	//
	
	graphEdit = in_graphEdit;
	nodeId = in_nodeId;
	
	auto node = graphEdit->tryGetNode(nodeId);
	
	Assert(node != nullptr);
	if (node != nullptr)
	{
		auto typeDefinition = graphEdit->typeDefinitionLibrary->tryGetTypeDefinition(node->typeName);
		
		Assert(typeDefinition != nullptr);
		if (typeDefinition != nullptr)
		{
			if (typeDefinition->resourceTypeName.empty() == false)
			{
				resourceTypeName = typeDefinition->resourceTypeName;
				if (typeDefinition->resourceEditor.create != nullptr)
					resourceEditor = typeDefinition->resourceEditor.create(typeDefinition->resourceEditor.createData);
				
				Assert(resourceEditor != nullptr);
				if (resourceEditor != nullptr)
				{
					resourceEditor->setResource(*node, resourceTypeName.c_str(), "editorData");
					
					resourceEditor->init(0, 0, resourceEditor->initSx, resourceEditor->initSy);
					
					window = new Window("Resource editor", resourceEditor->sx, resourceEditor->sy, true);
				}
			}
		}
	}
	
	return resourceEditor != nullptr;
}

void GraphEdit_NodeResourceEditorWindow::shut()
{
	delete window;
	window = nullptr;
	
	delete resourceEditor;
	resourceEditor = nullptr;
}

bool GraphEdit_NodeResourceEditorWindow::tick(const float dt)
{
	pushWindow(*window);
	{
		if (resourceEditor->sx != window->getWidth() ||
			resourceEditor->sy != window->getHeight())
		{
			resourceEditor->sx = window->getWidth();
			resourceEditor->sy = window->getHeight();
			
			resourceEditor->afterSizeChanged();
		}
		
		resourceEditor->tick(dt, false);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			resourceEditor->draw();
		}
		framework.endDraw();
	}
	popWindow();
	
	if (window->getQuitRequested())
	{
		save();
		
		return true;
	}
	else
	{
		return false;
	}
}

void GraphEdit_NodeResourceEditorWindow::save()
{
	auto node = graphEdit->tryGetNode(nodeId);
	
	if (node != nullptr)
	{
		std::string resourceData;
		
		if (resourceEditor->serializeResource(resourceData))
		{
			node->setResource(resourceTypeName.c_str(), "editorData", resourceData.c_str());
		}
		else
		{
			node->clearResource(resourceTypeName.c_str(), "editorData");
		}
	}
}
