#pragma once

#include "BoxAtlas.h"
#include "framework.h"

struct TextureAtlas
{
	BoxAtlas a;
	
	GLuint texture;
	
	TextureAtlas();
	~TextureAtlas();
	
	void init(const int sx, const int sy);
	
	BoxAtlasElem * tryAlloc(const uint8_t * values, const int sx, const int sy, const int border = 0);
	void free(BoxAtlasElem *& e);
	
	void clearTexture(GLuint texture, float r, float g, float b, float a);
	bool makeBigger(const int sx, const int sy);
	bool optimize();
};
