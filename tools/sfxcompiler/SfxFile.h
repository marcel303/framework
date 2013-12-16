#pragma once

#include "SfxHeader.h"

class SfxFile
{
public:
	SfxFile();
	~SfxFile();
	
	void Setup(SfxHeader header);
	
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	SfxHeader mHeader;
	uint8_t* mBytes;
};
