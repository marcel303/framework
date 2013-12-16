#pragma once

#include "Stream.h"
#include "VecComposition.h"

class CompositionIO
{
public:
	static void Load_Library(Stream* stream, ShapeLib& out_Library);
	static void Load_Composition(Stream* stream, Composition& out_Composition);
	static void Load(Stream* libStream, Stream* compStream, Composition& out_Composition);
};
