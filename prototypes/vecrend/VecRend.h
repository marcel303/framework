#pragma once

#include "Buffer.h"
#include "Shape.h"
#include "Stream.h"

enum DrawMode
{
	DrawMode_Regular,
	DrawMode_Stencil
};

Buffer* VecRend_CreateBuffer(Shape shape, int quality, DrawMode drawMode);
void VecCompile_Compile(Shape shape, std::string texFileName, std::string silFileName, Stream* stream);
