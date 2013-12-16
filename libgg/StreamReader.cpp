#include "Precompiled.h"
#include <string.h>
#include "Exception.h"
#include "Stream.h"
#include "StreamReader.h"

StreamReader::StreamReader(Stream* stream, bool takeOwnership)
{
	m_Stream = stream;
	m_IsOwner = takeOwnership;
}

StreamReader::~StreamReader()
{
	if (m_IsOwner)
	{
		delete m_Stream;
		m_Stream = 0;
	}
}

bool StreamReader::EOF_get()
{
	return m_Stream->EOF_get();
}

uint8_t* StreamReader::ReadAllBytes()
{
	const int byteCount = m_Stream->Length_get();

	uint8_t* bytes = new uint8_t[byteCount];

	if (m_Stream->Read(bytes, byteCount) != byteCount)
	{
		delete[] bytes;
		bytes = 0;
		throw ExceptionVA("Stream IO error");
	}

	return bytes;
}

std::string StreamReader::ReadAllText()
{
	std::string result;
	
	while (!EOF_get())
	{
		std::string line = ReadLine();

		result.append(line);
	}
	
	return result;
}

std::vector<std::string> StreamReader::ReadAllLines()
{
	std::vector<std::string> result;
	
	while (!EOF_get())
	{
		std::string line = ReadLine();

		result.push_back(line);
	}
	
	return result;
}

int8_t StreamReader::ReadInt8()
{
	int8_t v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

int16_t StreamReader::ReadInt16()
{
	int16_t v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

int32_t StreamReader::ReadInt32()
{
	int32_t v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

uint8_t StreamReader::ReadUInt8()
{
	uint8_t v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

uint16_t StreamReader::ReadUInt16()
{
	uint16_t v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

uint32_t StreamReader::ReadUInt32()
{
	uint32_t v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

float StreamReader::ReadFloat()
{
	float v;

	if (m_Stream->Read(&v, sizeof(v)) != sizeof(v))
		throw ExceptionVA("Stream IO error");

	return v;
}

void StreamReader::ReadBytes(void* bytes, uint32_t byteCount)
{
	if (m_Stream->Read(bytes, byteCount) != (int)byteCount)
		throw ExceptionVA("Stream IO error");
}

std::string StreamReader::ReadText_Binary()
{
	std::string v;

	uint32_t length = ReadUInt32();
	
#ifdef DEBUG
	if (length > 1000000)
		throw ExceptionVA("invalid text length: %d", (int)length);
#endif
	
	v.resize(length);

	m_Stream->Read(&v[0], length);

	return v;
}

void StreamReader::ReadText_Binary(char* out_String, size_t maxLength)
{
	std::string temp = ReadText_Binary();
	
	if (temp.length() + 1 > maxLength)
		throw ExceptionVA("string too long");
	
	strcpy(out_String, temp.c_str());
}

std::string StreamReader::ReadLine()
{
	std::string result;

	bool first = true;
	bool stop = false;

	while (!stop)
	{
		char c;

		int count = m_Stream->Read(&c, 1);

		if (count == 0)
		{
			if (first)
				throw ExceptionVA("cannot read beyond EOF");
			else
				stop = true;
		}
		else if (c == '\n')
		{
			char c2;
			
			if (m_Stream->Read(&c2, 1))
			{
				if (c2 != '\r')
					m_Stream->Seek(-1, SeekMode_Offset);
			}
			
			stop = true;
		}
		else if (c == '\r')
		{
			char c2;
			
			if (m_Stream->Read(&c2, 1))
			{
				if (c2 != '\n')
					m_Stream->Seek(-1, SeekMode_Offset);
			}
			
			stop = true;
		}
		else
			result.push_back(c);
		
		first = false;
	}

	return result;
}

Stream* StreamReader::Stream_get()
{
	return m_Stream;
}
