#pragma once

#include <stdint.h>
#include <vector>
#include "Stream.h"

class ArrayStream : public Stream
{
public:
	ArrayStream(const void* bytes, int byteCount);
	virtual ~ArrayStream();
	
	virtual void Open(const char* fileName, OpenMode mode);
	virtual void Close();
	virtual void Write(const void* bytes, int byteCount); // note: always throws
	virtual int Read(void* bytes, int byteCount);
	virtual int Length_get();
	virtual int Position_get();
	virtual void Seek(int seek, SeekMode mode);
	virtual bool EOF_get();
	
private:
	int ByteCount_get();
	
	const uint8_t* mBytes;
	int mByteCount;
	int mPosition;
};
