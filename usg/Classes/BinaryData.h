#pragma once

#include <stdint.h>
#include "Stream.h"

class BinaryData
{
public:
	BinaryData();
	~BinaryData();
	void Initialize();
	
	void Load(Stream* stream);
	
	uint8_t* m_Bytes;
	int m_ByteCount;
};
