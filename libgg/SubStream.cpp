#include "Exception.h"
#include "SubStream.h"

SubStream::SubStream(Stream* stream, bool own, int position, int length)
{
	m_Stream = stream;
	m_Own = own;
	m_Position = position;
	m_Length = length;
}

SubStream::~SubStream()
{
	if (m_Own)
	{
		delete m_Stream;
		m_Stream = 0;
	}
}

void SubStream::Open(const char* fileName, OpenMode mode)
{
	throw ExceptionVA("operation not supported");
}

void SubStream::Close()
{
	throw ExceptionVA("operation not supported");
}

void SubStream::Write(const void* bytes, int byteCount)
{
	throw ExceptionVA("operation not supported");
}

int SubStream::Read(void* bytes, int byteCount)
{
	if (Position_get() + byteCount > m_Length)
		byteCount = m_Length - Position_get();
	
	if (byteCount <= 0)
		return 0;

	return m_Stream->Read(bytes, byteCount);
}

int SubStream::Length_get()
{
	return m_Length;
}

int SubStream::Position_get()
{
	return m_Stream->Position_get() - m_Position;
}

void SubStream::Seek(int seek, SeekMode mode)
{
	if (mode == SeekMode_Offset)
		seek += Position_get();
	
	if (seek < 0 || seek > m_Length)
		throw ExceptionVA("seek out of range");
	
	seek += m_Position;

	m_Stream->Seek(seek, SeekMode_Begin);
}

bool SubStream::EOF_get()
{
	return Position_get() == m_Length;
}
