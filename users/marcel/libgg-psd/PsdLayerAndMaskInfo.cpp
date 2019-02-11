#include "Debugging.h"
#include "PsdLayer.h"
#include "PsdLayerAndMaskInfo.h"
#include "PsdLog.h"
#include "PsdTypes.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdLayerAndMaskInfo::PsdLayerAndMaskInfo()
{
	mLayerCount = 0;
	mSkipAlpha = false;
	mIsWritePrepared = false;
}

PsdLayerAndMaskInfo::~PsdLayerAndMaskInfo()
{
	for (size_t i = 0; i < mLayerList.size(); ++i)
		delete mLayerList[i];
	mLayerList.clear();
}

void PsdLayerAndMaskInfo::Setup(bool skipAlpha)
{
	mSkipAlpha = skipAlpha;
}

void PsdLayerAndMaskInfo::Read(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	const uint32_t length = SwapU32(reader.ReadUInt32());
	const uint32_t endPos = stream->Position_get() + length;
	
	const uint32_t layerLength = SwapU32(reader.ReadUInt32());
	const uint32_t layerEndPos = stream->Position_get() + layerLength;

	PSD_LOG_DBG("read: layer_begin: %d", stream->Position_get());

	mLayerCount = Swap16(reader.ReadInt16());

	mSkipAlpha = mLayerCount >= 0;

	if (mLayerCount < 0)
		mLayerCount = -mLayerCount;

	Assert(mLayerCount <= 100);
	
	PSD_LOG_DBG("layer_count: %d", mLayerCount);
	PSD_LOG_DBG("skip_alpha: %d", mSkipAlpha ? 1 : 0);
	PSD_LOG_DBG("reading layer descriptions");
	
	for (int i = 0; i < mLayerCount; ++i)
	{
		PsdLayer* layer = new PsdLayer();

		layer->ReadHeader(pi, stream);

		mLayerList.push_back(layer);
	}
	
	PSD_LOG_DBG("reading layer data: begin: %d", stream->Position_get());

	for (int i = 0; i < mLayerCount; ++i)
	{
		mLayerList[i]->ReadData(pi, stream);
	}

	stream->Seek(layerEndPos, SeekMode_Begin);
	stream->Seek(endPos, SeekMode_Begin);
}

void PsdLayerAndMaskInfo::WritePrepare()
{
	mIsWritePrepared = true;
	
	mLayerCount = (int16_t)mLayerList.size();
	
	for (size_t i = 0; i < mLayerList.size(); ++i)
	{
		mLayerList[i]->WritePrepare();
	}
}

void PsdLayerAndMaskInfo::Write(PsdInfo* pi, Stream* stream)
{
	Assert(mIsWritePrepared);
	
	StreamWriter writer(stream, false);

	writer.WriteUInt32(0); // stub length
	const uint32_t begin = stream->Position_get();
	
	writer.WriteUInt32(0); // stub layer length
	const uint32_t layerBegin = stream->Position_get();
	
	PSD_LOG_DBG("write: layer_begin: %d", layerBegin);
	PSD_LOG_DBG("write: layer_count: %d", mLayerCount);
	PSD_LOG_DBG("write: skip_alpha: %d", mSkipAlpha ? 1 : 0);
	
	if (mSkipAlpha)
		writer.WriteInt16(Swap16(+mLayerCount));
	else
		writer.WriteInt16(Swap16(-mLayerCount));
	
	PSD_LOG_DBG("writing layer descriptions");
	
	for (size_t i = 0; i < mLayerList.size(); ++i)
	{
		mLayerList[i]->WriteHeader(pi, stream);
	}

	PSD_LOG_DBG("writing layer data: begin: %d", stream->Position_get());

	for (size_t i = 0; i < mLayerList.size(); ++i)
	{
		mLayerList[i]->WriteData(pi, stream);
	}

	// calculate layer length
	const uint32_t layerEnd = stream->Position_get();
	const uint32_t layerLength = layerEnd - layerBegin;
	// write layer length field	
	stream->Seek(layerBegin - sizeof(layerLength), SeekMode_Begin);
	writer.WriteUInt32(SwapU32(layerLength));
	stream->Seek(layerEnd, SeekMode_Begin);

	// calculate length
	const uint32_t end = stream->Position_get();
	const uint32_t length = end - begin;
	// write length field
	stream->Seek(begin - sizeof(length), SeekMode_Begin);
	writer.WriteUInt32(SwapU32(length));
	stream->Seek(end, SeekMode_Begin);
}
