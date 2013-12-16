#pragma once

#include "ResIndex.h"
#include "Stream.h"

class ResIndexCompiler
{
public:
	static void Validate(ResIndex& index);
//	static void CopyResources(ResIndex& index, std::string dstPath);
	static void Compile(ResIndex& index, std::string name, std::string headerFileName, std::string platform, Stream* indexStream, Stream* headerStream, Stream* sourceStream);
};
