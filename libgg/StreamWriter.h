#pragma once

#include <string>
#include "libgg_forward.h"
#include "Types.h"

class StreamWriter
{
public:
	StreamWriter(Stream* stream, bool takeOwnership);
	~StreamWriter();

	void WriteInt8(int8_t v);
	void WriteInt16(int16_t v);
	void WriteInt32(int32_t v);
	void WriteUInt8(uint8_t v);
	void WriteUInt16(uint16_t v);
	void WriteUInt32(uint32_t v);
	void WriteFloat(float v);
	void WriteBytes(const void* bytes, uint32_t byteCount);

	// todo: endianness

	void WriteText(const char* str);
	void WriteText(const std::string& str);
	void WriteText_Binary(const char* str);
	void WriteText_Binary(const std::string& str);
	void WriteLine();
	void WriteLine(const char* str);
	void WriteLine(const std::string& str);

	Stream* Stream_get();

private:
	Stream* m_Stream;
	bool m_IsOwner;
};
