#include <string.h>
#include "ArrayStream.h"
#include "Exception.h"

ArrayStream::ArrayStream(const void* bytes, int byteCount) : Stream()
{
	mBytes = (const uint8_t*)bytes;
	mByteCount = byteCount;
	mPosition = 0;
}

ArrayStream::~ArrayStream()
{
}

void ArrayStream::Open(const char* fileName, OpenMode mode)
{
	throw ExceptionVA("operation not supported");
}

void ArrayStream::Close()
{
	throw ExceptionVA("operation not supported");
}

void ArrayStream::Write(const void* bytes, int byteCount)
{
	throw ExceptionVA("operation not supported");
}

int ArrayStream::Read(void* bytes, int byteCount)
{
//	printf("p=%d, bc=%d, bcg=%d\n", m_Position, byteCount, ByteCount_get());
	
	int length = ByteCount_get();

	if (mPosition + byteCount > length)
		byteCount = length - mPosition;
	
	if (byteCount <= 0)
		return 0;
	
	memcpy(bytes, mBytes + mPosition, byteCount);
	
	mPosition += byteCount;
	
	return byteCount;
}

int ArrayStream::Length_get()
{
	return ByteCount_get();
}

int ArrayStream::Position_get()
{
	return mPosition;
}

void ArrayStream::Seek(int seek, SeekMode mode)
{
	if (mode == SeekMode_Offset)
		seek += mPosition;
	
	if (seek < 0 || seek > ByteCount_get())
		throw ExceptionVA("seek out of range");
	
	mPosition = seek;
}

bool ArrayStream::EOF_get()
{
	return mPosition == ByteCount_get();
}

int ArrayStream::ByteCount_get()
{
	return mByteCount;
}
