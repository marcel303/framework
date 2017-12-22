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

#pragma once

#include "graph.h"

struct AudioGraph;
struct AudioGraphGlobals;
struct AudioValueHistorySet;

struct SDL_mutex;

struct AudioRealTimeConnection : GraphEdit_RealTimeConnection
{
	AudioGraph * audioGraph;
	AudioGraph ** audioGraphPtr;
	
	SDL_mutex * audioMutex;
	
	bool isLoading;
	
	AudioValueHistorySet * audioValueHistorySet;
	
	AudioGraphGlobals * globals;
	
	AudioRealTimeConnection(AudioValueHistorySet * audioValueHistorySet, AudioGraphGlobals * globals);
	virtual ~AudioRealTimeConnection() override;
	
	void updateAudioValues();
	
	virtual void loadBegin() override;
	virtual void loadEnd(GraphEdit & graphEdit) override;
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override;
	virtual void nodeRemove(const GraphNodeId nodeId) override;

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override;
	
	static bool setPlugValue(AudioPlug * plug, const std::string & value);
	static bool getPlugValue(AudioPlug * plug, std::string & value);

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
	virtual int linkIsActive(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override;
	
	virtual int getNodeCpuHeatMax() const override;
	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const override;
};

//

struct AudioFloat;

struct AudioValueHistory
{
	static const int kHistorySize = 8;
	static const int kNumSamples = AUDIO_UPDATE_SIZE * kHistorySize;
	
	uint64_t lastUpdateTime;
	
	float samples[kNumSamples];
	
	bool isValid;
	
	AudioValueHistory();
	
	void provide(const AudioFloat & value);
	
	bool isActive() const;
	
	AudioValueHistory(const AudioValueHistory & other) = delete;
	AudioValueHistory & operator=(AudioValueHistory const & other) = delete;
};

struct AudioValueHistory_SocketRef
{
	GraphNodeId nodeId;
	int srcSocketIndex;
	int dstSocketIndex;
	
	AudioValueHistory_SocketRef()
		: nodeId(kGraphNodeIdInvalid)
		, srcSocketIndex(-1)
		, dstSocketIndex(-1)
	{
	}
	
	bool operator<(const AudioValueHistory_SocketRef & other) const
	{
		if (nodeId != other.nodeId)
			return nodeId < other.nodeId;
		else if (srcSocketIndex != other.srcSocketIndex)
			return srcSocketIndex < other.srcSocketIndex;
		else
			return dstSocketIndex < other.dstSocketIndex;
	}
};

struct AudioValueHistorySet
{
	std::map<AudioValueHistory_SocketRef, AudioValueHistory> s_audioValues;
};
