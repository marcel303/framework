#include <string.h>
#include "Exception.h"
#include "MemoryStream.h"

MemoryStream::MemoryStream() : Stream()
{
	Initialize();
}

MemoryStream::MemoryStream(int byteCount) : Stream()
{
	Initialize();

	m_Bytes.resize(byteCount);

	memset(&m_Bytes[0], 0, byteCount);
}

MemoryStream::MemoryStream(const void* bytes, int byteCount) : Stream()
{
	Initialize();

	m_Bytes.reserve(byteCount);

	Write(bytes, byteCount);

	Seek(0, SeekMode_Begin);
}

MemoryStream::~MemoryStream()
{
}

void MemoryStream::Initialize()
{
	m_Position = 0;
}

void MemoryStream::Open(const char* fileName, OpenMode mode)
{
	throw ExceptionVA("operation not supported");
}

void MemoryStream::Close()
{
	throw ExceptionVA("operation not supported");
}

void MemoryStream::Write(const void* bytes, int byteCount)
{
	uint8_t* _bytes = (uint8_t*)bytes;
	
	int endIndex = m_Position + byteCount;
	
	if (endIndex > ByteCount_get())
	{
		m_Bytes.resize(endIndex);
	}
		
	for (int i = 0; i < byteCount; ++i)
	{
		m_Bytes[m_Position++] = _bytes[i];
	}
}

int MemoryStream::Read(void* bytes, int byteCount)
{
//	printf("p=%d, bc=%d, bcg=%d\n", m_Position, byteCount, ByteCount_get());
	
	int length = ByteCount_get();

	if (m_Position + byteCount > length)
		byteCount = length - m_Position;
	
	if (byteCount <= 0)
		return 0;
	
	memcpy(bytes, &m_Bytes[m_Position], byteCount);
	
	m_Position += byteCount;
	
	return byteCount;
}

int MemoryStream::Length_get()
{
	return ByteCount_get();
}

int MemoryStream::Position_get()
{
	return m_Position;
}

void MemoryStream::Seek(int seek, SeekMode mode)
{
	if (mode == SeekMode_Offset)
		seek += m_Position;
	
	if (seek < 0 || seek > ByteCount_get())
		throw ExceptionVA("seek out of range");
	
	m_Position = seek;
}

bool MemoryStream::EOF_get()
{
	return m_Position == ByteCount_get();
}

void MemoryStream::CopyFrom(Stream* stream)
{
	Seek(0, SeekMode_Begin);
	stream->Seek(0, SeekMode_Begin);
	
	m_Bytes.clear();
	
	const int length = stream->Length_get();
	
	m_Bytes.resize(length);
		
	if (length > 0 && stream->Read(&m_Bytes[0], length) != ByteCount_get())
		throw ExceptionVA("unable to read all bytes");
}

void MemoryStream::CopyTo(Stream* stream)
{
	Seek(0, SeekMode_Begin);
	stream->Seek(0, SeekMode_Begin);
	
	stream->Write(&m_Bytes[0], ByteCount_get());
}

void MemoryStream::StreamFrom(Stream* stream, int byteCount)
{
	m_Bytes.clear();
	
	m_Bytes.resize(byteCount);
	
	if (byteCount > 0)
	{
		const int readByteCount = stream->Read(&m_Bytes[0], byteCount);

		if (readByteCount != ByteCount_get())
			throw ExceptionVA("unable to read all bytes");
	}
}

void MemoryStream::StreamTo(Stream* stream, int byteCount)
{
	uint8_t* bytes = new uint8_t[byteCount];
	
	const int readByteCount = Read(bytes, byteCount);
	
	if (readByteCount != byteCount)
		throw ExceptionVA("unable to read all bytes");
	
	stream->Write(bytes, byteCount);
	
	delete[] bytes;
	bytes = 0;
}

void MemoryStream::ToArray(uint8_t** bytes, int* byteCount)
{
	*byteCount = Length_get();
	
	*bytes = new uint8_t[*byteCount];
	
	int position = Position_get();
	
	Seek(0, SeekMode_Begin);
	
	Read(*bytes, *byteCount);
	
	Seek(position, SeekMode_Begin);
}

void MemoryStream::FromArray(uint8_t* bytes, int byteCount)
{
	Seek(0, SeekMode_Begin);
	
	m_Bytes.clear();
	
	m_Bytes.reserve(byteCount);
	
	Write(bytes, byteCount);

	Seek(0, SeekMode_Begin);
}

const uint8_t* MemoryStream::Bytes_get() const
{
	return &m_Bytes[0];
}

int MemoryStream::ByteCount_get()
{
	return (int)m_Bytes.size();
}
