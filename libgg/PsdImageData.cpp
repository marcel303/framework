#include "Debugging.h"
#include "PsdChannel.h"
#include "PsdImageData.h"
#include "PsdInfo.h"
#include "PsdLog.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdImageData::PsdImageData()
{
	mCompression = PsdCompressionType_Raw;
}

void PsdImageData::Setup(Image* image, PsdCompressionType compression)
{
	assert(compression == PsdCompressionType_Rle);
	
	mImage.SetSize(image->m_Sx, image->m_Sy);
	image->Blit(&mImage, 0, 0);
	mCompression = compression;
}

void PsdImageData::Read(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	mCompression = (PsdCompressionType)SwapU16(reader.ReadUInt16());

	PSD_LOG_DBG("compression: %hu", mCompression);

	PsdChannel** channelList = new PsdChannel*[pi->mHeaderInfo.mChannelCount];
	
	for (int i = 0; i < pi->mHeaderInfo.mChannelCount; ++i)
	{
		channelList[i] = new PsdChannel();
		
		channelList[i]->ReadChannelHeader(stream, mCompression, pi->mHeaderInfo.mSy);
	}
	
	for (int i = 0; i < pi->mHeaderInfo.mChannelCount; ++i)
	{		
		channelList[i]->ReadChannelData(stream, mCompression, pi->mHeaderInfo.mMode, pi->mHeaderInfo.mSx, pi->mHeaderInfo.mSy);
		
		delete channelList[i];
	}
	
	delete[] channelList;
}

void PsdImageData::Write(PsdInfo* pi, Stream* stream)
{
	assert((int)pi->mHeaderInfo.mSx == mImage.m_Sx);
	assert((int)pi->mHeaderInfo.mSy == mImage.m_Sy);

	StreamWriter writer(stream, false);

	assert(mCompression == PsdCompressionType_Rle);

	writer.WriteUInt16(SwapU16(mCompression));

	PsdChannel** channelList = new PsdChannel*[pi->mHeaderInfo.mChannelCount];
	
	for (int i = 0; i < pi->mHeaderInfo.mChannelCount; ++i)
	{
		channelList[i] = new PsdChannel();
		
		// create channel

		int usage = i == 4 ? -1 : i;

		channelList[i]->Setup(mImage, usage);
		
		// compress
		
		channelList[i]->WritePrepare(mCompression, pi->mHeaderInfo.mMode, true);
		
		channelList[i]->WriteChannelHeader(stream, mCompression);
	}
	
	for (int i = 0; i < pi->mHeaderInfo.mChannelCount; ++i)
	{
		// write
		
		channelList[i]->WriteChannelData(stream);
		
		delete channelList[i];
	}
	
	delete[] channelList;
}
