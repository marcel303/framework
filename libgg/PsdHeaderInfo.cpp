#include <string.h>
#include "Debugging.h"
#include "PsdHeaderInfo.h"
#include "PsdLog.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

void PsdHeaderInfo::Read(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	stream->Read(mSignature, 4);
	mVersion = SwapU16(reader.ReadUInt16());
	char reserved[6];
	stream->Read(reserved, 6);
	mChannelCount = SwapU16(reader.ReadUInt16());
	mSy = SwapU32(reader.ReadUInt32());
	mSx = SwapU32(reader.ReadUInt32());
	mBpp = SwapU16(reader.ReadUInt16());
	mMode = (PsdImageMode)SwapU16(reader.ReadUInt16());

	if (memcmp(mSignature, "8BPS", 4))
		throw ExceptionVA("signature not recognized");
	if (mVersion != 1)
		throw ExceptionVA("unknown version: %lu", mVersion);
	if (mMode != PsdImageMode_Rgb)
		throw ExceptionVA("unknown image mode: %d", (int)mMode);

	PSD_LOG_DBG("header_info: read: version: %hu", mVersion);
	PSD_LOG_DBG("header_info: read: channelCount: %hu", mChannelCount);
	PSD_LOG_DBG("header_info: read: sx: %lu", mSx);
	PSD_LOG_DBG("header_info: read: sy: %lu", mSy);
	PSD_LOG_DBG("header_info: read: bpp: %hu", mBpp);
	PSD_LOG_DBG("header_info: read: mode: %hu", mMode);
}

void PsdHeaderInfo::Write(PsdInfo* pi, Stream* stream)
{
	StreamWriter writer(stream, false);

	stream->Write(mSignature, 4);
	writer.WriteUInt16(SwapU16(mVersion));
	char reserved[6];
	memset(reserved, 0, 6);
	stream->Write(reserved, 6);
	writer.WriteUInt16(SwapU16(mChannelCount));
	writer.WriteUInt32(SwapU32(mSy));
	writer.WriteUInt32(SwapU32(mSx));
	writer.WriteUInt16(SwapU16(mBpp));
	writer.WriteUInt16(SwapU16(mMode));

	PSD_LOG_DBG("header_info: write: version: %hu", mVersion);
	PSD_LOG_DBG("header_info: write: channelCount: %hu", mChannelCount);
	PSD_LOG_DBG("header_info: write: sx: %lu", mSx);
	PSD_LOG_DBG("header_info: write: sy: %lu", mSy);
	PSD_LOG_DBG("header_info: write: bpp: %hu", mBpp);
	PSD_LOG_DBG("header_info: write: mode: %hu", mMode);
}

void PsdHeaderInfo::Setup(const std::string& signature, int version, int channelCount, int sx, int sy, int bpp, PsdImageMode mode)
{
	assert(sx >= 0);
	assert(sy >= 0);

	if (signature != "8BPS")
		throw ExceptionVA("signature not recognized");
	if (version != 1)
		throw ExceptionVA("unknown version: %lu", version);
	if (bpp != 8)
		throw ExceptionVA("bpp must be 8: %d", bpp);
	if (mode != PsdImageMode_Rgb)
		throw ExceptionVA("mode must be 3 (RGB color): %d", mode);

	memcpy(mSignature, signature.c_str(), 4);
	mVersion = version;
	mChannelCount = channelCount;
	mSx = sx;
	mSy = sy;
	mBpp = bpp;
	mMode = mode;
}
