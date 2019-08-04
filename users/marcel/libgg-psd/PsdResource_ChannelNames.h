#pragma once

#include <vector>
#include "PsdResource.h"

class PsdResource_ChannelNames : public PsdImageResource
{
public:
	PsdResource_ChannelNames();
	virtual ~PsdResource_ChannelNames();
	void Setup(const std::string& name);
	
	void Add(const std::string& name);
	
	virtual void ReadResource(PsdInfo* pi, Stream* stream);
	virtual void WriteResource(PsdInfo* pi, Stream* stream);
	
	std::vector<std::string> mNameList;
};
