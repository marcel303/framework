#pragma once

#include "CompiledComposition.h"
#include "VecComposition.h"

class CompositionCompiler
{
public:
	static void Compile(const Composition& composition, VRCC::CompiledComposition& out_Composition);
};
