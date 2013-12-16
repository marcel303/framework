#pragma once

#include "CompiledShape.h"
#include "Shape.h"

namespace VRCS
{
	class ShapeCompiler
	{
	public:
		static void Compile(Shape shape, std::string texFileName, std::string silFileName, CompiledShape& out_Shape);
	};
};
