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

#pragma once

#include "graphEdit_realTimeConnection.h"

struct VfxGraph;

struct SavedVfxMemory;

struct RealTimeConnection : GraphEdit_RealTimeConnection
{
	VfxGraph * vfxGraph;
	VfxGraph ** vfxGraphPtr;
	
	VfxGraphContext * vfxGraphContext;
	
	bool isLoading;
	SavedVfxMemory * savedVfxMemory;
	
	RealTimeConnection(VfxGraph *& in_vfxGraph)
		: GraphEdit_RealTimeConnection()
		, vfxGraph(nullptr)
		, vfxGraphPtr(nullptr)
		, vfxGraphContext(nullptr)
		, isLoading(false)
		, savedVfxMemory(nullptr)
	{
		vfxGraph = in_vfxGraph;
		vfxGraphPtr = &in_vfxGraph;
		
		if (vfxGraph == nullptr)
			vfxGraphContext = new VfxGraphContext();
		else
			vfxGraphContext = vfxGraph->context;
		vfxGraphContext->refCount++;
	}
	
	~RealTimeConnection()
	{
		vfxGraphContext->refCount--;
		if (vfxGraphContext->refCount == 0)
			delete vfxGraphContext;
		vfxGraphContext = nullptr;
	}
	
	virtual void loadBegin() override;
	virtual void loadEnd(GraphEdit & graphEdit) override;
	
	virtual void saveBegin(GraphEdit & graphEdit) override;
	
	virtual void nodeAdd(const GraphNode & node) override;
	virtual void nodeInit(const GraphNode & node) override;
	virtual void nodeRemove(const GraphNodeId nodeId) override;

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	virtual void setLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name, const std::string & value) override;
	virtual void clearLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name) override;
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override;
	
	static bool setPlugValue(VfxGraph * vfxGraph, VfxPlug * plug, const std::string & value);
	static bool getPlugValue(VfxGraph * vfxGraph, VfxPlug * plug, std::string & value);

	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override;
	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override;
	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override;
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override;
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override;
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels) override;
	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels) override;
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override;
	
	virtual bool getNodeIssues(const GraphNodeId nodeId, std::vector<std::string> & issues) override;
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines) override;
	
	virtual int getNodeActivity(const GraphNodeId nodeId) override;
	virtual int getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	
	virtual bool getNodeDynamicSockets(const GraphNodeId nodeId, std::vector<DynamicInput> & inputs, std::vector<DynamicOutput> & outputs) const override;
	
	virtual int getNodeCpuHeatMax() const override;
	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const override;
	
	virtual int getNodeGpuHeatMax() const override;
	virtual int getNodeGpuTimeUs(const GraphNodeId nodeId) const override;
	
	virtual int getNodeImage(const GraphNodeId nodeId) const override;
};
