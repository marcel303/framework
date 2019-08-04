#include "Debugging.h"
#include "PsdLog.h"
#include "PsdTypes.h"
#include "PsdResource.h"
#include "PsdResourceList.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdImageResourceList::~PsdImageResourceList()
{
	for (size_t i = 0; i < mResourceList.size(); ++i)
		delete mResourceList[i];
	mResourceList.clear();
}

void PsdImageResourceList::Read(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	const uint32_t length = SwapU32(reader.ReadUInt32());

	PSD_LOG_DBG("image resource list: read: length: %lu", length);

	const int end = stream->Position_get() + length;

	while (stream->Position_get() < end)
	{
		PsdImageResource* resource = PsdImageResource::Read(pi, stream);

		mResourceList.push_back(resource);
	}

	Assert(stream->Position_get() == end);

	stream->Seek(end, SeekMode_Begin);
}

void PsdImageResourceList::Write(PsdInfo* pi, Stream* stream)
{
	StreamWriter writer(stream, false);

	writer.WriteUInt32(0); // stub length
	const int begin = stream->Position_get();

	for (size_t i = 0; i < mResourceList.size(); ++i)
	{
		mResourceList[i]->Write(pi, stream);
	}

	const int end = stream->Position_get();
	const uint32_t length = end - begin;
	
	PSD_LOG_DBG("image resource list: write: length: %lu", length);

	stream->Seek(begin - sizeof(length), SeekMode_Begin);
	writer.WriteUInt32(SwapU32(length));
	stream->Seek(end, SeekMode_Begin);
}
