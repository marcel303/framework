#pragma once

#include <string>
#include "MemoryStream.h"

class DataHeader
{
public:
	DataHeader();
	DataHeader(const char * type, const char * name);
	
	void Read(Stream * stream);
	void Write(Stream * stream, const int byteCount);
	
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
	DataStreamWriter(Stream * stream, const bool takeOwnership);
	~DataStreamWriter();
	
	void WriteSegment(const char * type, const char * name, const void * bytes, const int byteCount, const bool updateStreamPosition);
	
private:
	Stream * mStream;
	bool mTakeOwnership;
};

class DataStreamReader
{
public:
	DataStreamReader(Stream * stream, const bool takeOwnership);
	~DataStreamReader();
	
	DataHeader ReadHeader();
	void SkipSegment(const DataHeader & header);
	DataSegment * ReadSegment(const DataHeader & header);
	
private:
	Stream * mStream;
	bool mTakeOwnership;
};
