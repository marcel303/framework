#include "PsdColorModeData.h"
#include "PsdLog.h"
#include "PsdTypes.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

void PsdColorModeData::Read(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	const uint32_t length = SwapU32(reader.ReadUInt32());

	PSD_LOG_DBG("color mode data: read: length: %lu", length);

	// skip color mode data

	stream->Seek(length, SeekMode_Offset);
}

void PsdColorModeData::Write(PsdInfo* pi, Stream* stream)
{
	StreamWriter writer(stream, false);

	const uint32_t length = 0;

	writer.WriteUInt32(SwapU32(length));

	//for (uint32_t i = 0; i < length; ++i)
	//	writer.WriteUInt8(0);

	PSD_LOG_DBG("color mode data: write: length: %lu", length);
}
