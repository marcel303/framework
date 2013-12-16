#include "SfxHeader.h"
#include "StreamReader.h"
#include "StreamWriter.h"

SfxHeader::SfxHeader()
{
	mChannelCount = 0;
	mBitsPerChannel = 0;
	mFrameCount = 0;
	mFrameRate = 0;
	mDuration = 0.0f;
}

void SfxHeader::Setup(uint8_t channelCount, uint8_t bitsPerChannel, uint32_t frameCount, uint32_t frameRate)
{
	mChannelCount = channelCount;
	mBitsPerChannel = bitsPerChannel;
	mFrameCount = frameCount;
	mFrameRate = frameRate;
	
	mDuration = (float)frameCount / (float)frameRate;
	mByteCount = channelCount * frameCount * bitsPerChannel / 8;
}

void SfxHeader::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	mChannelCount = reader.ReadUInt8();
	mBitsPerChannel = reader.ReadUInt8();
	mFrameCount = reader.ReadUInt32();
	mFrameRate = reader.ReadUInt32();
	mDuration = reader.ReadFloat();
}

void SfxHeader::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	writer.WriteUInt8(mChannelCount);
	writer.WriteUInt8(mBitsPerChannel);
	writer.WriteUInt32(mFrameCount);
	writer.WriteUInt32(mFrameRate);
	writer.WriteFloat(mDuration);
}
