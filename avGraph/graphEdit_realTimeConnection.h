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

#include "graph.h"
#include "graph_typeDefinitionLibrary.h" // for Graph_EnumDefinition::Elem used by dynamic inputs

struct GraphEdit;

struct GraphEdit_ChannelData
{
	struct Channel
	{
		const float * values;
		int numValues;
		bool continuous;
		
		Channel()
			: values(nullptr)
			, numValues(0)
			, continuous(false)
		{
		}
	};
	
	std::vector<Channel> channels;
	
	GraphEdit_ChannelData()
		: channels()
	{
	}
	
	void addChannel(const float * values, const int numValues, const bool continuous)
	{
		Channel channel;
		channel.values = values;
		channel.numValues = numValues;
		channel.continuous = continuous;
		
		channels.push_back(channel);
	}
	
	void clear()
	{
		channels.clear();
	}
	
	bool hasChannels() const
	{
		return !channels.empty();
	}
};

struct GraphEdit_RealTimeConnection
{
	enum ActivityFlags
	{
		kActivity_Inactive    = 0,
		kActivity_OneShot     = 1 << 0,
		kActivity_Continuous  = 1 << 1
	};
	
	struct DynamicInput
	{
		std::string name;
		std::string typeName;
		std::string defaultValue;
		
		std::vector<Graph_EnumDefinition::Elem> enumElems;
	};
	
	struct DynamicOutput
	{
		std::string name;
		std::string typeName;
	};
	
	virtual ~GraphEdit_RealTimeConnection()
	{
	}
	
	virtual void loadBegin()
	{
	}
	
	virtual void loadEnd(GraphEdit & graphEdit)
	{
	}
	
	virtual void saveBegin(GraphEdit & graphEdit)
	{
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName)
	{
	}
	
	virtual void nodeRemove(const GraphNodeId nodeId)
	{
	}
	
	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
	{
	}
	
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
	{
	}
	
	virtual void setLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name, const std::string & value)
	{
	}
	
	virtual void clearLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name)
	{
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough)
	{
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value)
	{
	}
	
	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value)
	{
		return false;
	}
	
	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value)
	{
	}
	
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value)
	{
		return false;
	}
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
	{
	}
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels)
	{
		return false;
	}
	
	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels)
	{
		return false;
	}
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
	{
	}
	
	virtual void handleDstSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName)
	{
	}
	
	virtual bool getNodeIssues(const GraphNodeId nodeId, std::vector<std::string> & issues)
	{
		return false;
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines)
	{
		return false;
	}
	
	virtual int getNodeActivity(const GraphNodeId nodeId)
	{
		return kActivity_Inactive;
	}
	
	virtual int getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex)
	{
		return kActivity_Inactive;
	}
	
	virtual bool getNodeDynamicSockets(const GraphNodeId nodeId, std::vector<DynamicInput> & inputs, std::vector<DynamicOutput> & outputs) const
	{
		return false;
	}
	
	virtual int getNodeCpuHeatMax() const
	{
		return 1000;
	}
	
	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const
	{
		return 0;
	}
	
	virtual int getNodeGpuHeatMax() const
	{
		return 1000;
	}
	
	virtual int getNodeGpuTimeUs(const GraphNodeId nodeId) const
	{
		return 0;
	}
	
	virtual int getNodeImage(const GraphNodeId nodeId) const
	{
		return 0;
	}
};
