#include <string.h>
#include "DdsLoader.h"
#include "Exception.h"
#include "Log.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

void DDS_PIXELFORMAT::Load(StreamReader& reader)
{
	dwSize = reader.ReadUInt32(); // must be 32

	if (dwSize != 32)
		throw ExceptionVA("DDS pixel format header size not 32 bytes");

	dwFlags = reader.ReadUInt32();
	if (reader.Stream_get()->Read(dwFourCC, 4) != 4)
		throw ExceptionVA("file read error");
	dwRGBBitCount = reader.ReadUInt32();
	dwRBitMask = reader.ReadUInt32();
	dwGBitMask = reader.ReadUInt32();
	dwBBitMask = reader.ReadUInt32();
	dwABitMask = reader.ReadUInt32();

	if (dwFlags & DDPF_FOURCC)
	{
		LOG_DBG("DDS file contains FOURCC pixel format", 0);
	}
}

void DDS_PIXELFORMAT::Save(StreamWriter& writer)
{
	if (dwSize != 32)
		throw ExceptionVA("DDS pixel format header size not 32 bytes");

	writer.WriteUInt32(dwSize); // must be 32
	writer.WriteUInt32(dwFlags);
	writer.Stream_get()->Write(dwFourCC, 4);
	writer.WriteUInt32(dwRGBBitCount);
	writer.WriteUInt32(dwRBitMask);
	writer.WriteUInt32(dwGBitMask);
	writer.WriteUInt32(dwBBitMask);
	writer.WriteUInt32(dwABitMask);
}

void DDS_HEADER::Load(StreamReader& reader)
{
	if (reader.Stream_get()->Read(magic, 4) != 4)
		throw ExceptionVA("file read error");

	if (memcmp(magic, "DDS ", 4))
		throw ExceptionVA("DDS file missing magic 'DDS ' value");

	dwSize = reader.ReadUInt32(); // must be 124

	if (dwSize != 124)
		throw ExceptionVA("DDS header size not 124 bytes");

	dwFlags = reader.ReadUInt32();
	dwHeight = reader.ReadUInt32();
	dwWidth = reader.ReadUInt32();
	dwPitchOrLinearSize = reader.ReadUInt32();
	dwDepth = reader.ReadUInt32();
	dwMipMapCount = reader.ReadUInt32();
	for (int i = 0; i < 11; ++i)
		dwReserved1[i] = reader.ReadUInt32();
	ddspf.Load(reader);
	dwCaps = reader.ReadUInt32();
	dwCaps2 = reader.ReadUInt32();
	dwCaps3 = reader.ReadUInt32();
	dwCaps4 = reader.ReadUInt32();
	dwReserved2 = reader.ReadUInt32();

	if ((dwFlags & DDSD_CAPS) == 0)
		throw ExceptionVA("missing CAPS flag");
	if ((dwFlags & DDSD_HEIGHT) == 0)
		throw ExceptionVA("missing height flag");
	if ((dwFlags & DDSD_WIDTH) == 0)
		throw ExceptionVA("missing width flag");
	if ((dwFlags & DDSD_PIXELFORMAT) == 0)
		throw ExceptionVA("missing pixel format flag");
	if ((dwFlags & DDSCAPS_TEXTURE) == 0)
		throw ExceptionVA("missing texture cap");
}

void DDS_HEADER::Save(StreamWriter& writer)
{
	if (dwSize != 124)
		throw ExceptionVA("DDS header size not 124 bytes");
	if (memcmp(magic, "DDS ", 4))
		throw ExceptionVA("DDS file missing magic 'DDS ' value");

	writer.Stream_get()->Write(magic, 4);
	writer.WriteUInt32(dwSize); // must be 124
	writer.WriteUInt32(dwFlags);
	writer.WriteUInt32(dwHeight);
	writer.WriteUInt32(dwWidth);
	writer.WriteUInt32(dwPitchOrLinearSize);
	writer.WriteUInt32(dwDepth);
	writer.WriteUInt32(dwMipMapCount);
	for (int i = 0; i < 11; ++i)
		writer.WriteUInt32(dwReserved1[i]);
	ddspf.Save(writer);
	writer.WriteUInt32(dwCaps);
	writer.WriteUInt32(dwCaps2);
	writer.WriteUInt32(dwCaps3);
	writer.WriteUInt32(dwCaps4);
	writer.WriteUInt32(dwReserved2);
}

void DdsLoader::LoadHeader(StreamReader& reader)
{
	mHeader.Load(reader);
}

void DdsLoader::SeekToData(int fileStartPosition, StreamReader& reader)
{
	reader.Stream_get()->Seek(fileStartPosition + 128, SeekMode_Offset);
}

bool DdsLoader::IsFourCC(const char* name)
{
	if ((mHeader.ddspf.dwFlags & DDPF_FOURCC) == 0)
		return false;

	return !memcmp(mHeader.ddspf.dwFourCC, name, 4);
}

void DdsWriter::WriteHeader(StreamWriter& writer)
{
	mHeader.Save(writer);
}
