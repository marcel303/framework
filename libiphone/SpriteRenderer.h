#pragma once

#include "Types.h"

class SpriteRenderer
{
public:
	static void DrawTriangles_VCT_Index(
		void* vertexArray,
		int vertexStride,
		void* colorArray,
		int colorStride,
		void* textureArray,
		int textureStride,
		uint16_t* indexArray, 
		int indexCount);
	
	static void DrawTriangles_V3N3CT_Index(
		void* vertexArray,
		int vertexStride,
		void* normalArray,
		int normalStride,
		void* colorArray,
		int colorStride,
		void* textureArray,
		int textureStride,
		uint16_t* indexArray, 
		int indexCount);
};
