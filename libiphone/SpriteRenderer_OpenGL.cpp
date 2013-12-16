#include "OpenGLCompat.h"
#include "SpriteRenderer.h"

void SpriteRenderer::DrawTriangles_VCT_Index(void* vertexArray, int vertexStride, void* colorArray, int colorStride, void* textureArray, int textureStride, uint16_t* indexArray, int indexCount)
{
	// set buffers
	
	glVertexPointer(2, GL_FLOAT, vertexStride, vertexArray);
	glEnableClientState(GL_VERTEX_ARRAY);
	GL_CHECKERROR();
	glDisableClientState(GL_NORMAL_ARRAY);
	GL_CHECKERROR();
	glColorPointer(4, GL_UNSIGNED_BYTE, colorStride, colorArray);
	glEnableClientState(GL_COLOR_ARRAY);
	GL_CHECKERROR();
	glTexCoordPointer(2, GL_FLOAT, textureStride, textureArray);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_CHECKERROR();
	
	// emit draw call
	
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, indexArray);
	GL_CHECKERROR();
}

void SpriteRenderer::DrawTriangles_V3N3CT_Index(void* vertexArray, int vertexStride, void* normalArray, int normalStride, void* colorArray, int colorStride, void* textureArray, int textureStride, uint16_t* indexArray, int indexCount)
{
	// set buffers
	
	glVertexPointer(3, GL_FLOAT, vertexStride, vertexArray);
	glEnableClientState(GL_VERTEX_ARRAY);
	GL_CHECKERROR();
	
	if (normalArray)
	{
		glNormalPointer(GL_FLOAT, normalStride, normalArray);
		glEnableClientState(GL_NORMAL_ARRAY);
		GL_CHECKERROR();
	}
	else
	{
		glDisableClientState(GL_NORMAL_ARRAY);
		GL_CHECKERROR();
	}
	
	if (colorArray)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, colorStride, colorArray);
		glEnableClientState(GL_COLOR_ARRAY);
		GL_CHECKERROR();
	}
	else
	{
		glDisableClientState(GL_COLOR_ARRAY);
		GL_CHECKERROR();
	}
	
	glTexCoordPointer(2, GL_FLOAT, textureStride, textureArray);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_CHECKERROR();
	
	// emit draw call
	
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, indexArray);
	GL_CHECKERROR();
}
