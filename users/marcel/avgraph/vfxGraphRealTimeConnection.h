#pragma once

#include "graph.h"

struct VfxGraph;

struct RealTimeConnection : GraphEdit_RealTimeConnection
{
	VfxGraph * vfxGraph;
	VfxGraph ** vfxGraphPtr;
	
	bool isLoading;
	
	RealTimeConnection()
		: GraphEdit_RealTimeConnection()
		, vfxGraph(nullptr)
		, vfxGraphPtr(nullptr)
		, isLoading(false)
	{
	}
	
	virtual void loadBegin() override;
	virtual void loadEnd(GraphEdit & graphEdit) override;
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override;
	virtual void nodeRemove(const GraphNodeId nodeId) override;

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override;
	
	static bool setPlugValue(VfxPlug * plug, const std::string & value);
	static bool getPlugValue(VfxPlug * plug, std::string & value);

	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override;
	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override;
	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override;
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override;
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override;
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels) override;
	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels) override;
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override;
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines) override;
	
	virtual int nodeIsActive(const GraphNodeId nodeId) override;
	virtual int linkIsActive(const GraphLinkId linkId) override;
};
