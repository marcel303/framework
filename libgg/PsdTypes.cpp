#include "Debugging.h"
#include "PsdTypes.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

std::string PsdPascalString::Read(Stream* stream, int padding)
{
	StreamReader reader(stream, false);
	
	std::string result;
	
	const uint8_t length = reader.ReadUInt8();

	for (int i = 0; i < length; ++i)
	{
		char c = reader.ReadInt8();
		
		result.push_back(c);
	}
	
	// word align
	
	//if (length % 2)
	while (stream->Position_get() % padding)
		reader.ReadUInt8();
	
	return result;
}

void PsdPascalString::Write(Stream* stream, const std::string& text, int padding)
{
	assert(text.length() <= 255);
	assert(padding >= 1);
	
	StreamWriter writer(stream, false);
	
	const uint8_t length = (uint8_t)text.length();

	writer.WriteUInt8(length);

	for (int i = 0; i < length; ++i)
	{
		const char c = text[i];
		
		writer.WriteInt8(c);
	}

	writer.WriteInt8(0);

	// word align
	
	while (stream->Position_get() % padding)
		writer.WriteInt8(0);
}
