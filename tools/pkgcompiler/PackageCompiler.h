#pragma once

#include <string>
#include <vector>
#include "CompiledPackage.h"

class PackageCompiler
{
public:
	static void Compile(std::vector<std::string> srcList, std::string dst, CompiledPackage& out_Package);
};
