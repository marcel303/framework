#pragma once

#include "BoxAtlas.h"
#include "framework.h"

struct TextureAtlas
{
	BoxAtlas a;
	
	GLuint texture;
	GLenum internalFormat;
	
	bool filter;
	bool clamp;
	
	GLint swizzleMask[4];
	
	TextureAtlas();
	~TextureAtlas();
	
	void init(const int sx, const int sy, const GLenum internalFormat, const bool filter, const bool clamp, const GLint * swizzleMask);
	
	BoxAtlasElem * tryAlloc(const uint8_t * values, const int sx, const int sy, const GLenum uploadFormat, const GLenum uploadType, const int border = 0);
	void free(BoxAtlasElem *& e);
	
	GLuint allocateTexture(const int sx, const int sy);
	void clearTexture(GLuint texture, float r, float g, float b, float a);
	
	bool makeBigger(const int sx, const int sy);
	bool optimize();
	bool makeBiggerAndOptimize(const int sx, const int sy);
};
