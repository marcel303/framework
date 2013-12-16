#include <stdio.h>
#include <string.h>
#include "Archive.h"
#include "Debugging.h"
#include "Parse.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

#include "StringEx.h"

Archive::Archive()
{
	Initialize();
}

Archive::~Archive()
{
	if (m_StreamReader || m_StreamWriter)
		Close();
}

void Archive::Initialize()
{
	m_Log = LogCtx("archive");
	m_StreamReader = 0;
	m_StreamWriter = 0;
}

void Archive::OpenRead(Stream* stream)
{
	Assert(!m_StreamReader && !m_StreamWriter);
	
	m_StreamReader = new StreamReader(stream, false);
}

void Archive::OpenWrite(Stream* stream)
{
	Assert(!m_StreamReader && !m_StreamWriter);
	
	m_StreamWriter = new StreamWriter(stream, false);
}

void Archive::Close()
{
	Assert(m_StreamReader || m_StreamWriter);
	
	delete m_StreamReader;
	m_StreamReader = 0;

	delete m_StreamWriter;
	m_StreamWriter = 0;
}

bool Archive::NextSection()
{
	if (!Read())
		return false;
	
	return m_LineType == LineType_Section;
}

bool Archive::NextValue()
{
	if (!Read())
		return false;
	
	return m_LineType == LineType_Value;
}

const char* Archive::GetSection() const
{
	return m_Section;
}

const char* Archive::GetKey() const
{
	return m_Key;
}

const char* Archive::GetValue() const
{
	return m_Value;
}

int Archive::GetValue_Int32() const
{
	return Parse::Int32(m_Value);
}

float Archive::GetValue_Float() const
{
	return Parse::Float(m_Value);
}

void Archive::WriteSection(const char* section)
{
	//m_Log.WriteLine(LogLevel_Debug, "section: %s", section);

	m_StreamWriter->WriteUInt8((uint8_t)LineType_Section);
	m_StreamWriter->WriteText_Binary(section);
}

void Archive::WriteSectionEnd()
{
	m_StreamWriter->WriteUInt8((uint8_t)LineType_SectionEnd);

	//m_Log.WriteLine(LogLevel_Debug, "section [end]");
}

void Archive::WriteValue(const char* key, const char* value)
{
	m_StreamWriter->WriteUInt8((uint8_t)LineType_Value);
	m_StreamWriter->WriteText_Binary(key);
	m_StreamWriter->WriteText_Binary(value);
}

void Archive::WriteValue_Int32(const char* key, int value)
{
	WriteValue(key, String::FormatC("%d", value).c_str());
}

void Archive::WriteValue_Float(const char* key, float value)
{
	WriteValue(key, String::FormatC("%8.3f", value).c_str());
}

void Archive::WriteEOF()
{
	m_StreamWriter->WriteUInt8((uint8_t)LineType_EOF);
}

bool Archive::Read()
{
	uint8_t lineType;
	
	lineType = m_StreamReader->ReadUInt8();
	
	m_LineType = (LineType)lineType;
	
	switch (m_LineType)
	{
		case LineType_Section:
		{
			m_StreamReader->ReadText_Binary(m_Section, sizeof(m_Section));
			return true;
		}
		case LineType_SectionEnd:
			return true;
		case LineType_Value:
		{
			m_StreamReader->ReadText_Binary(m_Key, sizeof(m_Key));
			m_StreamReader->ReadText_Binary(m_Value, sizeof(m_Value));
			return true;
		}
		case LineType_EOF:
			return true;

		default:
#if !defined(DEPLOYMENT)
			throw ExceptionVA("unknown line type: %d", (int)m_LineType);
#else
			return false;
#endif
	}
}
