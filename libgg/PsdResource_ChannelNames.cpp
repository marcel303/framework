#include "PsdResource_ChannelNames.h"
#include "PsdTypes.h"

PsdResource_ChannelNames::PsdResource_ChannelNames() : PsdImageResource()
{
}

PsdResource_ChannelNames::~PsdResource_ChannelNames()
{
}

void PsdResource_ChannelNames::Setup(const std::string& name)
{
	PsdImageResource::Setup("8BIM", PsdResourceId_ChannelNames, name);
}

void PsdResource_ChannelNames::Add(const std::string& name)
{
	mNameList.push_back(name);
}

void PsdResource_ChannelNames::ReadResource(PsdInfo* pi, Stream* stream)
{
	throw ExceptionNA();
}

void PsdResource_ChannelNames::WriteResource(PsdInfo* pi, Stream* stream)
{
	for (size_t i = 0; i < mNameList.size(); ++i)
	{
		PsdPascalString::Write(stream, mNameList[i].c_str(), 1);
	}
}
