#pragma once

#include <string>
#include "MemoryStream.h"

class DataHeader
{
public:
	DataHeader();
	DataHeader(const char* type, const char* name);
	
	void Read(Stream* stream);
	void Write(Stream* stream, int byteCount);
	
	std::string mType;
	std::string mName;
	
	uint32_t mStreamPosition;
	uint32_t mDataStreamPosition;
	uint32_t mSegmentLength;
	uint32_t mDataLength;
};

class DataSegment
{
public:
	MemoryStream mData;
};

class DataStreamWriter
{
public:
	DataStreamWriter(Stream* stream, bool takeOwnership);
	~DataStreamWriter();
	
	void WriteSegment(const char* type, const char* name, void* bytes, int byteCount, bool updateStreamPosition);
	
private:
	Stream* mStream;
	bool mTakeOwnership;
};

class DataStreamReader
{
public:
	DataStreamReader(Stream* stream, bool takeOwnership);
	~DataStreamReader();
	
	DataHeader ReadHeader();
	void SkipSegment(DataHeader header);
	DataSegment* ReadSegment(DataHeader header);
	
private:
	Stream* mStream;
	bool mTakeOwnership;
};
