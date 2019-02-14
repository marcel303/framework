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
#include "vfxGraph.h"
#include "vfxGraphManager.h"
#include "vfxGraphRealTimeConnection.h"

VfxGraphInstance::~VfxGraphInstance()
{
	delete realTimeConnection;
	realTimeConnection = nullptr;
	
	delete vfxGraph;
	vfxGraph = nullptr;
}


//

VfxGraphManager_Basic::VfxGraphManager_Basic(const bool in_cacheOnCreate)
	: typeDefinitionLibrary(nullptr)
	, graphCache()
	, cacheOnCreate(false)
	, instances()
{
	cacheOnCreate = in_cacheOnCreate;
	
	//
	
	init();
}

VfxGraphManager_Basic::~VfxGraphManager_Basic()
{
	shut();
}

void VfxGraphManager_Basic::init()
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createVfxTypeDefinitionLibrary(*typeDefinitionLibrary);
}

void VfxGraphManager_Basic::shut()
{
	Assert(instances.empty());
	
	while (!instances.empty())
	{
		auto instance = instances.front();
		
		free(instance);
	}
	
	//
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
	
	for (auto & i : graphCache)
		delete i.second.graph;
	graphCache.clear();
}

void VfxGraphManager_Basic::addGraphToCache(const char * filename)
{
	auto i = graphCache.find(filename);
	
	if (i == graphCache.end())
	{
		auto e = graphCache.emplace(filename, GraphCacheElem());
		
		auto & elem = e.first->second;
		
		elem.graph = new Graph();
		elem.isValid = elem.graph->load(filename, typeDefinitionLibrary);
	}
}

void VfxGraphManager_Basic::selectFile(const char * filename)
{
}

void VfxGraphManager_Basic::selectInstance(const VfxGraphInstance * instance)
{
}

VfxGraphInstance * VfxGraphManager_Basic::createInstance(const char * filename, const int sx, const int sy)
{
	VfxGraph * vfxGraph = nullptr;
	
	auto graphItr = graphCache.find(filename);
	
	if (graphItr != graphCache.end())
	{
		auto & elem = graphItr->second;
		
		if (elem.isValid)
		{
			vfxGraph = constructVfxGraph(*elem.graph, typeDefinitionLibrary);
		}
	}
	else
	{
		if (cacheOnCreate)
		{
			auto e = graphCache.emplace(filename, GraphCacheElem());
			
			auto & elem = e.first->second;
			
			elem.graph = new Graph();
			elem.isValid = elem.graph->load(filename, typeDefinitionLibrary);
			
			if (elem.isValid)
			{
				vfxGraph = constructVfxGraph(*elem.graph, typeDefinitionLibrary);
			}
		}
		else
		{
			Graph graph;
			
			if (graph.load(filename, typeDefinitionLibrary))
			{
				vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
			}
		}
	}
	
	if (vfxGraph == nullptr)
	{
		vfxGraph = new VfxGraph();
	}
	
	VfxGraphInstance * instance = new VfxGraphInstance();
	
	instance->vfxGraph = vfxGraph;
	instance->sx = sx;
	instance->sy = sy;
	
	instances.push_back(instance);
	
	return instance;
}

void VfxGraphManager_Basic::free(VfxGraphInstance *& instance)
{
	auto i = std::find(instances.begin(), instances.end(), instance);
	
	if (i != instances.end())
		instances.erase(i);
	
	delete instance;
	instance = nullptr;
}

void VfxGraphManager_Basic::tick(const float dt)
{
	for (auto instance : instances)
		instance->vfxGraph->tick(instance->sx, instance->sy, dt);
}

void VfxGraphManager_Basic::tickVisualizers(const float dt)
{
}

void VfxGraphManager_Basic::traverseDraw() const
{
	for (auto instance : instances)
		instance->texture = instance->vfxGraph->traverseDraw(instance->sx, instance->sy);
}

bool VfxGraphManager_Basic::tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured)
{
	return false;
}

void VfxGraphManager_Basic::drawEditor(const int sx, const int sy)
{
}

//

struct VfxGraphFileRTC : GraphEdit_RealTimeConnection
{
	VfxGraphFile * file = nullptr;
	
	virtual void loadBegin() override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->loadBegin();
	}

	virtual void loadEnd(GraphEdit & graphEdit) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->loadEnd(graphEdit);
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->nodeAdd(nodeId, typeName);
	}

	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->nodeRemove(nodeId);
	}

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
	
	virtual void setLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name, const std::string & value) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setLinkParameter(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex, name, value);
	}
	
	virtual void clearLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->clearLinkParameter(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex, name);
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setNodeIsPassthrough(nodeId, isPassthrough);
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}

	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->clearSrcSocketValue(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getSrcSocketChannelData(nodeId, srcSocketIndex, srcSocketName, channels);
	}

	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getDstSocketChannelData(nodeId, dstSocketIndex, dstSocketName, channels);
	}
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		if (file->activeInstance != nullptr)
			file->activeInstance->realTimeConnection->handleSrcSocketPressed(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeDescription(nodeId, lines);
	}
	
	virtual bool getNodeIssues(const GraphNodeId nodeId, std::vector<std::string> & issues) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeIssues(nodeId, issues);
	}
	
	virtual int getNodeActivity(const GraphNodeId nodeId) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeActivity(nodeId);
	}

	virtual int getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getLinkActivity(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual bool getNodeDynamicSockets(const GraphNodeId nodeId, std::vector<DynamicInput> & inputs, std::vector<DynamicOutput> & outputs) const override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeDynamicSockets(nodeId, inputs, outputs);
	}
	
	virtual int getNodeCpuHeatMax() const override
	{
		if (file->activeInstance == nullptr)
			return 1000 * 1000;
		else
			return file->activeInstance->realTimeConnection->getNodeCpuHeatMax();
	}

	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const override
	{
		if (file->activeInstance == nullptr)
			return 0;
		else
			return file->activeInstance->realTimeConnection->getNodeCpuTimeUs(nodeId);
	}
	
	virtual int getNodeGpuHeatMax() const override
	{
		if (file->activeInstance == nullptr)
			return 1000 * 1000;
		else
			return file->activeInstance->realTimeConnection->getNodeGpuHeatMax();
	}
	
	virtual int getNodeGpuTimeUs(const GraphNodeId nodeId) const override
	{
		if (file->activeInstance == nullptr)
			return 0;
		else
			return file->activeInstance->realTimeConnection->getNodeGpuTimeUs(nodeId);
	}
};

VfxGraphFile::VfxGraphFile()
{
	realTimeConnection = new VfxGraphFileRTC();
	realTimeConnection->file = this;
}

//

VfxGraphManager_RTE::VfxGraphManager_RTE(const int in_displaySx, const int in_displaySy)
	: displaySx(in_displaySx)
	, displaySy(in_displaySy)
	, typeDefinitionLibrary(nullptr)
{
	init();
}

VfxGraphManager_RTE::~VfxGraphManager_RTE()
{
	shut();
}

void VfxGraphManager_RTE::init()
{
	shut();
	
	//
	
	typeDefinitionLibrary = new Graph_TypeDefinitionLibrary();
	
	createVfxTypeDefinitionLibrary(*typeDefinitionLibrary);
}

void VfxGraphManager_RTE::shut()
{
	for (auto & fileItr : files)
	{
		auto & file = fileItr.second;
		Assert(file->instances.empty());
		
		while (!file->instances.empty())
		{
			auto instance = file->instances.front();
			
			free(instance);
		}
		
		delete file;
		file = nullptr;
	}
	
	files.clear();
}

void VfxGraphManager_RTE::selectFile(const char * filename)
{
	VfxGraphFile * newSelectedFile = nullptr;
	
	if (filename != nullptr)
	{
		auto fileItr = files.find(filename);
		
		if (fileItr != files.end())
		{
			newSelectedFile = fileItr->second;
		}
	}
	
	if (newSelectedFile != selectedFile)
	{
		if (selectedFile != nullptr)
		{
			selectedFile->graphEdit->cancelEditing();
			
			selectedFile = nullptr;
		}
		
		//
		
		selectedFile = newSelectedFile;
		
		selectedFile->graphEdit->beginEditing();
	}
}

void VfxGraphManager_RTE::selectInstance(const VfxGraphInstance * instance)
{
	if (instance == nullptr)
	{
		selectFile(nullptr);
	}
	else
	{
		for (auto & fileItr : files)
		{
			auto file = fileItr.second;
			
			for (auto & instanceInFile : file->instances)
			{
				if (instance == instanceInFile)
				{
					selectFile(fileItr.first.c_str());
					
					file->activeInstance = instance;
				}
			}
		}
	}
}

VfxGraphInstance * VfxGraphManager_RTE::createInstance(const char * filename, const int sx, const int sy)
{
	VfxGraphFile * file = nullptr;
	
	auto fileItr = files.find(filename);
	
	if (fileItr != files.end())
	{
		file = fileItr->second;
	}
	else
	{
		file = new VfxGraphFile();
		file->filename = filename;
		
		file->graphEdit = new GraphEdit(displaySx, displaySy, typeDefinitionLibrary);
		file->graphEdit->realTimeConnection = file->realTimeConnection;
		
		file->graphEdit->load(filename);
		
		files[filename] = file;
	}

	//

	auto vfxGraph = constructVfxGraph(*file->graphEdit->graph, typeDefinitionLibrary);
	
	VfxGraphInstance * instance = new VfxGraphInstance();
	instance->vfxGraph = vfxGraph;
	instance->sx = sx;
	instance->sy = sy;
	
	auto realTimeConnection = new RealTimeConnection(instance->vfxGraph);
	instance->realTimeConnection = realTimeConnection;
	
	//

	file->instances.push_back(instance);

	//
	
	if (file->activeInstance == nullptr)
	{
		file->activeInstance = instance;
	}

	return instance;
}

void VfxGraphManager_RTE::free(VfxGraphInstance *& instance)
{
	if (instance == nullptr)
	{
		return;
	}
	
	bool found = false;
	
	for (auto fileItr = files.begin(); fileItr != files.end(); )
	{
		auto & file = fileItr->second;
		
		for (auto instanceItr = file->instances.begin(); instanceItr != file->instances.end(); )
		{
			if (*instanceItr == instance)
			{
				found = true;
				
				instanceItr = file->instances.erase(instanceItr);
				
				if (instance == file->activeInstance)
				{
					if (file->instances.empty())
					{
						file->activeInstance = nullptr;
					}
					else
					{
						file->activeInstance = file->instances.front();
					}
				}
			}
			else
			{
				instanceItr++;
			}
		}
		
		fileItr++;
	}
	
	Assert(found);
	
	delete instance;
	instance = nullptr;
}

void VfxGraphManager_RTE::tick(const float dt)
{
	for (auto & file_itr : files)
	{
		for (auto & instance : file_itr.second->instances)
		{
			instance->vfxGraph->tick(instance->sx, instance->sy, dt);
		}
	}
}

void VfxGraphManager_RTE::tickVisualizers(const float dt)
{
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->tickVisualizers(dt);
	}
}

void VfxGraphManager_RTE::traverseDraw() const
{
	for (auto & file_itr : files)
	{
		for (auto instance : file_itr.second->instances)
		{
			instance->texture = instance->vfxGraph->traverseDraw(instance->sx, instance->sy);
		}
	}
}

bool VfxGraphManager_RTE::tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured)
{
	bool result = false;
	
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->displaySx = sx;
		selectedFile->graphEdit->displaySy = sy;

		result |= selectedFile->graphEdit->tick(dt, isInputCaptured);
	}
	
	return result;
}

void VfxGraphManager_RTE::drawEditor(const int sx, const int sy)
{
	if (selectedFile != nullptr)
	{
		selectedFile->graphEdit->displaySx = sx;
		selectedFile->graphEdit->displaySy = sy;

		selectedFile->graphEdit->draw();
	}
}
