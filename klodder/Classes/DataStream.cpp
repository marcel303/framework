#include "DataStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

DataHeader::DataHeader()
{
	mStreamPosition = 0;
	mDataStreamPosition = 0;
	mSegmentLength = 0;
	mDataLength = 0;
}

DataHeader::DataHeader(const char* type, const char* name)
{
	mType = type;
	mName = name;
	mSegmentLength = 0;
}

void DataHeader::Read(Stream* stream)
{
	StreamReader reader(stream, false);
	
	mStreamPosition = stream->Position_get();
	mSegmentLength = reader.ReadUInt32();
	mType = reader.ReadText_Binary();
	mName = reader.ReadText_Binary();
	mDataStreamPosition = stream->Position_get();
	mDataLength = mSegmentLength - (mDataStreamPosition - mStreamPosition);
}

void DataHeader::Write(Stream* stream, int byteCount)
{
	uint32_t position1 = stream->Position_get();
	
	StreamWriter writer(stream, false);
	
	writer.WriteUInt32(0); // dummy segment length
	writer.WriteText_Binary(mType);
	writer.WriteText_Binary(mName);
	
	uint32_t position2 = stream->Position_get();
	
	mSegmentLength = position2 - position1 + byteCount;
	
	stream->Seek(position1, SeekMode_Begin);
	
	writer.WriteUInt32(mSegmentLength);
	
	stream->Seek(position2, SeekMode_Begin);
}

//

DataStreamWriter::DataStreamWriter(Stream* stream, bool takeOwnership)
{
	mStream = stream;
	mTakeOwnership = takeOwnership;
}

DataStreamWriter::~DataStreamWriter()
{
	if (mTakeOwnership)
		delete mStream;
}

void DataStreamWriter::WriteSegment(const char* type, const char* name, void* bytes, int byteCount, bool updateStreamPosition)
{
	int position = mStream->Position_get();
	
	DataHeader header(type, name);
	
	header.Write(mStream, byteCount);
	
	mStream->Write(bytes, byteCount);
	
	if (!updateStreamPosition)
		mStream->Seek(position, SeekMode_Begin);
}

//

DataStreamReader::DataStreamReader(Stream* stream, bool takeOwnership)
{
	mStream = stream;
	mTakeOwnership = takeOwnership;
}

DataStreamReader::~DataStreamReader()
{
	if (mTakeOwnership)
		delete mStream;
}

DataHeader DataStreamReader::ReadHeader()
{
	DataHeader header;
	
	header.Read(mStream);
	
	return header;
}

void DataStreamReader::SkipSegment(DataHeader header)
{
	mStream->Seek(header.mStreamPosition + header.mSegmentLength, SeekMode_Begin);
}

DataSegment* DataStreamReader::ReadSegment(DataHeader header)
{
	DataSegment* result = new DataSegment();
	
	mStream->Seek(header.mDataStreamPosition, SeekMode_Begin);
	
	result->mData.StreamFrom(mStream, header.mDataLength);
	
	mStream->Seek(header.mStreamPosition + header.mSegmentLength, SeekMode_Begin);
	
	return result;
}
