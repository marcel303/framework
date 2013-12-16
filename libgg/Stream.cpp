#include "Precompiled.h"
#include <stdint.h>
#include "Debugging.h"
#include "Exception.h"
#include "Stream.h"

Stream::~Stream()
{
}

void StreamExtensions::StreamTo(Stream* src, Stream* dst, int bufferSize, int length)
{
	Assert(bufferSize > 0);
	
	uint8_t* bytes = new uint8_t[bufferSize];
	
	for (int todo = length; todo;)
	{
		const int byteCount = todo > bufferSize ? bufferSize : todo;
		
		if (src->Read(bytes, byteCount) != byteCount)
			throw ExceptionVA("failed to read %d bytes from stream", byteCount);
		
		dst->Write(bytes, byteCount);
		
		todo -= byteCount;
	}
	
	delete[] bytes;
	bytes = 0;
}

void StreamExtensions::StreamTo(Stream* src, Stream* dst, int bufferSize)
{
	const int length = src->Length_get() - src->Position_get();
	
	StreamTo(src, dst, bufferSize, length);
}

void StreamExtensions::WriteTo(Stream* src, Stream* dst)
{
	src->Seek(0, SeekMode_Begin);

	int byteCount = src->Length_get();

	if (byteCount == 0)
		return;

	uint8_t* bytes = new uint8_t[byteCount];

	src->Read(bytes, byteCount);

	dst->Write(bytes, byteCount);

	delete[] bytes;
	bytes = 0;
}
