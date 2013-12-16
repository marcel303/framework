#include "Brush_Pattern.h"
#include "BrushLibrary.h"
#include "MemoryStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

enum ChunkType
{
	ChunkType_Brush = 10
};

class Chunk
{
public:
	void Setup(ChunkType _type, uint8_t _version)
	{
		type = _type;
		version = _version;
	}

	void Load(Stream* stream, bool loadData)
	{
		StreamReader reader(stream, false);

		type = (ChunkType)reader.ReadUInt8();
		version = reader.ReadUInt8();
		length = reader.ReadUInt32();

		if (loadData)
		{
			// load data

			uint8_t* bytes = new uint8_t[length];

			stream->Read(bytes, length);

			data.Write(bytes, length);

			delete[] bytes;

			data.Seek(0, SeekMode_Begin);
		}
		else
		{
			// skip data

			stream->Seek(length, SeekMode_Offset);
		}
	}

	void Save(Stream* stream)
	{
		length = data.Length_get();

		StreamWriter writer(stream, false);

		// write header

		writer.WriteUInt8((uint8_t)type);
		writer.WriteUInt8(version);
		writer.WriteUInt32(length);

		// write data

		uint8_t* bytes = new uint8_t[data.Length_get()];

		data.Seek(0, SeekMode_Begin);

		data.Read(bytes, data.Length_get());

		stream->Write(bytes, data.Length_get());

		delete[] bytes;
	}

	ChunkType type;
	uint8_t version;
	uint32_t length;
	MemoryStream data;
};

BrushLibrary::~BrushLibrary()
{
	Clear();
}

void BrushLibrary::Clear()
{
	for (size_t i = 0; i < mBrushList.size(); ++i)
		delete mBrushList[i];

	mBrushList.clear();
}

void BrushLibrary::Load(Stream* stream, bool loadData)
{
	Assert(loadData);
	
	Clear();

	//

	while (!stream->EOF_get())
	{
		Chunk chunk;

		chunk.Load(stream, true);

		switch (chunk.type)
		{
		case ChunkType_Brush:
			{
				Brush_Pattern* brush = new Brush_Pattern();

				brush->Load(&chunk.data, chunk.version, loadData);

				Append(brush, true);
				break;
			}

		default:
			throw ExceptionVA("unknown chunk type: %d", (int)chunk.type);
		}
	}
}

void BrushLibrary::Save(Stream* stream)
{
	// save brushes

	for (size_t i = 0; i < mBrushList.size(); ++i)
	{
		AppendChunk(stream, mBrushList[i]);
	}
}

void BrushLibrary::Append(Brush_Pattern* brush, bool takeOwnership)
{
	if (takeOwnership != true)
		throw ExceptionVA("must take ownership");

	mBrushList.push_back(brush);
}

void BrushLibrary::AppendChunk(Stream* stream, Brush_Pattern* brush)
{
	stream->Seek(stream->Length_get(), SeekMode_Begin);
	
	int version = 1;

	// build chunk

	Chunk chunk;

	chunk.Setup(ChunkType_Brush, version);

	brush->Save(&chunk.data, version);

	// write chunk

	chunk.Save(stream);
}

Brush_Pattern* BrushLibrary::Find(uint32_t patternId)
{
	for (size_t i = 0; i < mBrushList.size(); ++i)
		if (mBrushList[i]->mPatternId == patternId)
			return mBrushList[i];

	return 0;
}
