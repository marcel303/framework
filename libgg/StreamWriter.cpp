#include "Precompiled.h"
#include <string>
#include <string.h>
#include "Stream.h"
#include "StreamWriter.h"
#include "StringEx.h"

StreamWriter::StreamWriter(Stream* stream, bool takeOwnership)
{
	m_Stream = stream;
	m_IsOwner = takeOwnership;
}

StreamWriter::~StreamWriter()
{
	if (m_IsOwner)
	{
		delete m_Stream;
		m_Stream = 0;
	}
}

void StreamWriter::WriteInt8(int8_t v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteInt16(int16_t v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteInt32(int32_t v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteUInt8(uint8_t v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteUInt16(uint16_t v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteUInt32(uint32_t v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteFloat(float v)
{
	m_Stream->Write(&v, sizeof(v));
}

void StreamWriter::WriteText(const char* str)
{
	m_Stream->Write(str, (int)strlen(str));
}

void StreamWriter::WriteText(const std::string& str)
{
	WriteText(str.c_str());
}

void StreamWriter::WriteText_Binary(const char* str)
{
	uint32_t length = (uint32_t)strlen(str);

	WriteUInt32(length);
	
	m_Stream->Write(str, length);
}

void StreamWriter::WriteText_Binary(const std::string& str)
{
	WriteText_Binary(str.c_str());
}

void StreamWriter::WriteLine()
{
	WriteLine("");
}

void StreamWriter::WriteLine(const char* str)
{
	WriteText(str);
	WriteText("\n");
}

void StreamWriter::WriteLine(const std::string& str)
{
	WriteLine(str.c_str());
}

Stream* StreamWriter::Stream_get()
{
	return m_Stream;
}
