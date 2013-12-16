#pragma once

#include "ByteString.h"
#endif
class WaveFile
{
public:
	WaveFile();

	void Load();
	void Save();

	ByteString mBytes;
};
