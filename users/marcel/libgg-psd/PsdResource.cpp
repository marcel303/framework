#include <string.h>
#include "PsdLog.h"
#include "PsdResource.h"
#include "PsdResource_DisplayInfo.h"
#include "PsdResource_ResolutionInfo.h"
#include "PsdTypes.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdImageResource::PsdImageResource()
{
	memset(mOsType, 0, 4);
	mId = 0;
}

PsdImageResource::~PsdImageResource()
{
}

void PsdImageResource::Setup(std::string osType, uint16_t id, const std::string& name)
{
	if (osType != "8BIM")
		throw ExceptionVA("OS type must be 8BIM");
	
	memcpy(mOsType, osType.c_str(), 4);
	mId = id;
	mName = name;
}

PsdImageResource* PsdImageResource::Read(PsdInfo* pi, Stream* stream)
{
	char osType[4];
	uint16_t id;
	std::string name;
	uint32_t length;

	//

	PsdImageResource* result = 0;

	StreamReader reader(stream, false);

	stream->Read(osType, 4);

	if (!memcmp(osType, "8BIM", 4))
	{
		id = SwapU16(reader.ReadUInt16());
		name = PsdPascalString::Read(stream, 2);
		//if (stream->Position_get() % 2)
		//	reader.ReadUInt8(); // unknown
		length = SwapU32(reader.ReadUInt32());

		// word align length..
		
		if (length % 2)
		{
			length++;
		}

		PSD_LOG_DBG("resource: read: id=%hu, name=%s, length=%lu", id, name.c_str(), length);

		const int begin = stream->Position_get();
		const int end = begin + length;

		switch (id)
		{
		case 1005: // resolution info
			{
				PsdResolutionInfo* resource = new PsdResolutionInfo();
				resource->ReadResource(pi, stream);
				result = resource;
				break;
			}
		case 1007: // display info
			{
				PsdDisplayInfo* resource = new PsdDisplayInfo();
				resource->ReadResource(pi, stream);
				result = resource;
				break;
			}
		case 1034: // copyright
			break;
		case 1033: // thumbnail
		case 1036:
			break;
		case 1037: // global angle
			break;
		case 1046: // color count
			break;
		case 1047: // transparent index
			break;
		default:
			//throw ExceptionVA("unknown resource id: {0}", id);
			break;
		}

		stream->Seek(end, SeekMode_Begin);
	}
	else
	{
		throw ExceptionVA("unknown OS type");
	}
	
	if (result)
	{
		result->mName = name;
	}

	return result;
}

void PsdImageResource::Write(PsdInfo* pi, Stream* stream)
{
	if (mId == 0)
		throw ExceptionVA("ID not set");
	if (memcmp(mOsType, "8BIM", 4))
		throw ExceptionVA("unknown OS type");
	
	StreamWriter writer(stream, false);
	
	stream->Write(mOsType, 4);
	
	PSD_LOG_DBG("resource: write: id=%hu, name=%s", mId, mName.c_str());
	
	writer.WriteUInt16(SwapU16(mId));

	PsdPascalString::Write(stream, mName, 2);
	//if (stream->Position_get() % 2)
		//writer.WriteUInt8(0); // unknown
	
	writer.WriteUInt32(SwapU32(0)); // size stub

	const int begin = stream->Position_get();
	
	WriteResource(pi, stream);

	const int end = stream->Position_get();
	const uint32_t length = end - begin;

	PSD_LOG_DBG("image resource: write: length: %lu", length);

	if (length % 2)
	{
		PSD_LOG_DBG("aligning to 2 bytes");
		writer.WriteUInt8(0); // align to 2 bytes
	}
	
	const int end2 = stream->Position_get();
	
	stream->Seek(begin - sizeof(length), SeekMode_Begin);
	writer.WriteUInt32(SwapU32(length));
	stream->Seek(end2, SeekMode_Begin);
}
