#include <kernel.h>
#include <libgu.h>
#include <libgum.h>
#include "Graphics.h"
#include "SpriteRenderer.h"

void SpriteRenderer::DrawTriangles_VCT_Index(
	void* vertexArray,
	int vertexStride,
	void* colorArray,
	int colorStride,
	void* textureArray,
	int textureStride,
	uint16_t* indexArray,
	int indexCount)
{
	// PSP code *requires* packed, interleaved vertex format
	// perform a few basic assertions to check up on this
	// component order: texcoord2 | rgba | [normal] | position3

	//int vertexSize = sizeof(float) * 2 + sizeof(uint32_t) + sizeof(float) * 3;

	//Assert(vertexStride == colorStride == textureStride == vertexSize);
	//Assert((vertexArray < colorArray) && (colorArray < textureArray));

	if (indexCount == 0)
		return;

	sceKernelDcacheWritebackAll();

	Assert(gGraphics.mRenderInProgress);

	sceGumDrawArray(
		SCEGU_PRIM_TRIANGLES,
		SCEGU_TEXTURE_FLOAT | SCEGU_VERTEX_FLOAT | SCEGU_COLOR_PF8888 | SCEGU_INDEX_USHORT,
		indexCount,
		indexArray,
		textureArray);
}
