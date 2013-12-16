#pragma once

#include <string.h>
#include "libgg_forward.h"
#include "Log.h"
#include "Types.h"

/*
 
 header format:
 
 type [strlen]
 
 0 = section + strlen
 1 = section end
 10 = value + strlen
 20 = EOF
 
*/

class Archive
{
public:
	Archive();
	~Archive();
	void Initialize();
	
	void OpenRead(Stream* stream);
	void OpenWrite(Stream* stream);
	void Close();
	
	bool NextSection();
	bool NextValue();
	
	inline bool IsSection(const char* section) const
	{
		return !strcmp(m_Section, section);
	}
	
	inline bool IsKey(const char* key) const
	{
		return !strcmp(m_Key, key);
	}

	inline bool TypeIsSection() const
	{
		return m_LineType == LineType_Section;
	}

	inline bool TypeIsSectionEnd() const
	{
		return m_LineType == LineType_SectionEnd;
	}

	inline bool TypeIsValue() const
	{
		return m_LineType == LineType_Value;
	}

	inline bool TypeIsEOF() const
	{
		return m_LineType == LineType_EOF;
	}
	
	const char* GetSection() const;
	const char* GetKey() const;
	const char* GetValue() const;
	int GetValue_Int32() const;
	float GetValue_Float() const;
	
	void WriteSection(const char* section);
	void WriteSectionEnd();
	void WriteValue(const char* key, const char* value);
	void WriteValue_Int32(const char* key, int value);
	void WriteValue_Float(const char* key, float value);
	void WriteEOF();
	
	bool Read();

private:
	bool ReadSection();
	bool ReadValue();
	
	enum LineType
	{
		LineType_Section = 0,
		LineType_SectionEnd = 1,
		LineType_Value = 10,
		LineType_EOF = 20
	};
	
	StreamWriter* m_StreamWriter;
	StreamReader* m_StreamReader;
	
	char m_Section[64];
	char m_Key[64];
	char m_Value[64];
	LineType m_LineType;
	LogCtx m_Log;
};
