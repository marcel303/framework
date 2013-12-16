#include "SfxFile.h"

SfxFile::SfxFile()
{
	mBytes = 0;
}

SfxFile::~SfxFile()
{
	delete[] mBytes;
	mBytes = 0;
}

void SfxFile::Setup(SfxHeader header)
{
	delete[] mBytes;
	mBytes = 0;
	
	mHeader = header;
	
	if (header.mByteCount > 0)
	{
		mBytes = new uint8_t[header.mByteCount];
	}
}

void SfxFile::Load(Stream* stream)
{
	mHeader.Load(stream);
	
	Setup(mHeader);
	
	stream->Read(mBytes, mHeader.mByteCount);
}

void SfxFile::Save(Stream* stream)
{
	mHeader.Save(stream);
	
	stream->Write(mBytes, mHeader.mByteCount);
}
