#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "Exception.h"
#include "libgg_forward.h"

class StreamReader
{
public:
	StreamReader(Stream* stream, bool takeOwnership);
	~StreamReader();
	bool EOF_get();

	uint8_t* ReadAllBytes();
	std::string ReadAllText();
	std::vector<std::string> ReadAllLines();
	
	int8_t ReadInt8();
	int16_t ReadInt16();
	int32_t ReadInt32();
	uint8_t ReadUInt8();
	uint16_t ReadUInt16();
	uint32_t ReadUInt32();
	float ReadFloat();
	void ReadBytes(void* bytes, uint32_t byteCount);

	std::string ReadText_Binary();
	void ReadText_Binary(char* out_String, size_t maxLength);
	std::string ReadLine();
	
	Stream* Stream_get();
	
private:
	Stream* m_Stream;
	bool m_IsOwner;
};
