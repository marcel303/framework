#include <string.h>
#include "Debugging.h"
#include "PsdChannel.h"
#include "PsdInfo.h"
#include "PsdLayer.h"
#include "PsdLog.h"
#include "PsdTypes.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdLayer::PsdLayer()
{
	mChannelCount = 0;
	mSignature[0] = mSignature[1] = mSignature[2] = mSignature[3] = 0;
	mBlendKey[0] = mBlendKey[1] = mBlendKey[2] = mBlendKey[3] = 0;
	mOpacity = 0;
	mClipping = 0;
	mFlags = 0;
	mWritePrepared = false;
}

PsdLayer::~PsdLayer()
{
	for (size_t i = 0; i < mChannelList.size(); ++i)
		delete mChannelList[i];
	mChannelList.clear();
}

void PsdLayer::Setup(const std::string& name, PsdRect rect, const std::string& signature, const std::string& blendKey, uint8_t opacity, uint8_t clipping, uint8_t flags)
{
	if (signature != "8BIM")
		throw ExceptionVA("signature mismatch");
	if (blendKey.length() != 4)
		throw ExceptionVA("blend key must have length 4");
	
	mName = name;
	mRect = rect;
	
	memcpy(mSignature, signature.c_str(), 4);
	memcpy(mBlendKey, blendKey.c_str(), 4);
	
	mOpacity = opacity;
	mClipping = clipping;
	mFlags = flags;
}

void PsdLayer::ReadHeader(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	mRect.Read(reader);

	mChannelCount = SwapU16(reader.ReadUInt16());

	assert(mChannelCount <= 10);

	PSD_LOG_DBG("layer: read: channelCount: %d", (int)mChannelCount);

	PSD_LOG_DBG("layer: reading channel headers");
	
	for (int i = 0; i < mChannelCount; ++i)
	{
		PsdChannel* channel = new PsdChannel();

		channel->ReadHeader(reader);

		mChannelList.push_back(channel);
	}

	stream->Read(mSignature, 4);

	if (memcmp(mSignature, "8BIM", 4))
		throw ExceptionVA("signature mismatch: %c%c%c%c", mSignature[0], mSignature[1], mSignature[2], mSignature[3]);
    
	stream->Read(mBlendKey, 4);

	mOpacity = reader.ReadUInt8();
	mClipping = reader.ReadUInt8();
	mFlags = reader.ReadUInt8();
	uint8_t padding = reader.ReadUInt8();

	PSD_LOG_DBG("layer: read: blendKey: %c%c%c%c", mBlendKey[0], mBlendKey[1], mBlendKey[2], mBlendKey[3]);
	PSD_LOG_DBG("layer: read: opacity: %d", (int)mOpacity);
	PSD_LOG_DBG("layer: read: clipping: %d", (int)mClipping);
	PSD_LOG_DBG("layer: read: flags: %d", (int)mFlags);
	PSD_LOG_DBG("layer: read: padding: %d", (int)padding);

	uint32_t dataSize = SwapU32(reader.ReadUInt32());
	uint32_t dataEndPos = stream->Position_get() + dataSize;
	
	PSD_LOG_DBG("layer: read: data_size: %d", (int)dataSize);

	uint32_t maskSize = SwapU32(reader.ReadUInt32());
	uint32_t maskEndPos = stream->Position_get() + maskSize;
	
	PSD_LOG_DBG("layer: read: maskSize: %d", (int)maskSize);
	
	stream->Seek(maskEndPos, SeekMode_Begin);

	uint32_t blendingSize = SwapU32(reader.ReadUInt32());
	uint32_t blendingEndPos = stream->Position_get() + blendingSize;
	
	PSD_LOG_DBG("layer: read: blendingSize: %d", (int)blendingSize);
	
	stream->Seek(blendingEndPos, SeekMode_Begin);

	mName = PsdPascalString::Read(stream, 2);

	PSD_LOG_DBG("layer: read: name: %s", mName.c_str());

	stream->Seek(dataEndPos, SeekMode_Begin);
}

void PsdLayer::ReadData(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	PSD_LOG_DBG("read_data: %d", (int)stream->Position_get());
	
	for (int i = 0; i < mChannelCount; ++i)
	{
		const uint16_t compression = SwapU16(reader.ReadUInt16());
		
		PSD_LOG_DBG("layer: read: compression: %d", (int)compression);

		PsdChannel* channel = new PsdChannel();
		
		channel->ReadChannelHeader(stream, compression, mRect.Sy_get());

		channel->ReadChannelData(stream, compression, pi->mHeaderInfo.mMode, mRect.Sx_get(), mRect.Sy_get());
		
		delete channel;
	}
}

void PsdLayer::WritePrepare()
{
	mWritePrepared = true;
	
	mChannelCount = (uint16_t)mChannelList.size();
}

void PsdLayer::WriteHeader(PsdInfo* pi, Stream* stream)
{
	assert(mWritePrepared);
	assert(!memcmp(mSignature, "8BIM", 4));

	//
	
	StreamWriter writer(stream, false);

	mRect.Write(writer);

	writer.WriteUInt16(SwapU16(mChannelCount));

	PSD_LOG_DBG("layer: write: channelCount: %d", (int)mChannelCount);

	const uint16_t compression = PsdCompressionType_Rle;

	for (size_t i = 0; i < mChannelCount; ++i)
	{
		mChannelList[i]->WritePrepare(compression, pi->mHeaderInfo.mMode, false);

		mChannelList[i]->WriteHeader(stream);
	}

	stream->Write(mSignature, 4);

	// 'levl'=levels
	// 'curv'=curves
	// 'brit'=brightness/contrast
	// 'blnc'=color balance
	// 'hue '=old hue/saturation, Photoshop 4.0 
	// 'hue2'=new hue/saturation, Photoshop 5.0 
	// 'selc'=selective color 
	// 'thrs'=threshold 
	// 'nvrt'=invert 
	// 'post'=posterize

	stream->Write(mBlendKey, 4);

	const uint8_t padding = 0;
	
	writer.WriteUInt8(mOpacity);
	writer.WriteUInt8(mClipping);
	writer.WriteUInt8(mFlags);
	writer.WriteUInt8(padding);

	PSD_LOG_DBG("layer: write: blendKey: %c%c%c%c", mBlendKey[0], mBlendKey[1], mBlendKey[2], mBlendKey[3]);
	PSD_LOG_DBG("layer: write: opacity: %d", (int)mOpacity);
	PSD_LOG_DBG("layer: write: clipping: %d", (int)mClipping);
	PSD_LOG_DBG("layer: write: flags: %d", (int)mFlags);
	PSD_LOG_DBG("layer: write: padding: %d", (int)padding);
					  
	// layer data

	writer.WriteUInt32(0); // stub data length

	const int dataBegin = stream->Position_get();

	const uint32_t maskSize = 0;
	const uint32_t blendingSize = 0;
	
	writer.WriteUInt32(maskSize); // mask size
	writer.WriteUInt32(blendingSize); // layer blending size

	PSD_LOG_DBG("layer: write: maskSize: %d", (int)maskSize);
	PSD_LOG_DBG("layer: write: blendingSize: %d", (int)blendingSize);

	// write layer name
	
	PsdPascalString::Write(stream, mName, 2);

	PSD_LOG_DBG("layer: write: name: %s", mName.c_str());

	const int dataEnd = stream->Position_get();

	const uint32_t dataSize = dataEnd - dataBegin;
	stream->Seek(dataBegin - sizeof(dataSize), SeekMode_Begin);
	writer.WriteUInt32(SwapU32(dataSize));
	stream->Seek(dataEnd, SeekMode_Begin);
}

void PsdLayer::WriteData(PsdInfo* pi, Stream* stream)
{
	assert(mWritePrepared);
	
	StreamWriter writer(stream, false);

	PSD_LOG_DBG("write_data: %d", (int)stream->Position_get());
	
	for (int i = 0; i < mChannelCount; ++i)
	{
		mChannelList[i]->Write(stream);
	}
}
