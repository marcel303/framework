#pragma once

#include <string>
#include "libgg_forward.h"

class FileStreamExtents
{
public:
	static bool ContentsAreEqual(Stream* src, Stream* dst);
	static void OverwriteIfChanged(MemoryStream* src, const std::string& fileName);
};
