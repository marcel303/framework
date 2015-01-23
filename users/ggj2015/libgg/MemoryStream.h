#pragma once

#include <stdint.h>
#include <vector>
#include "Stream.h"

class MemoryStream : public Stream
{
public:
	MemoryStream();
	MemoryStream(int byteCount);
	MemoryStream(const void* bytes, int byteCount);
	virtual ~MemoryStream();
	void Initialize();
	
	virtual void Open(const char* fileName, OpenMode mode);
	virtual void Close();
	virtual void Write(const void* bytes, int byteCount); // note: always throws
	virtual int Read(void* bytes, int byteCount);
	virtual int Length_get();
	virtual int Position_get();
	virtual void Seek(int seek, SeekMode mode);
	virtual bool EOF_get();
	
	void CopyFrom(Stream* stream); // resets seek position on source & destination streams
	void CopyTo(Stream* stream); // resets seek position on source & destination streams
	
	void StreamFrom(Stream* stream, int byteCount);
	void StreamTo(Stream* stream, int byteCount);

	void ToArray(uint8_t** bytes, int* byteCount);
	void FromArray(uint8_t* bytes, int byteCount);

	const uint8_t* Bytes_get() const;
	
private:
	int ByteCount_get();
	
	std::vector<uint8_t> m_Bytes;
	int m_Position;
};
