#pragma once

#include "CompiledFont.h"
#include "Atlas_ImageInfo.h"
#include "TextureAtlas.h"

class FontMap
{
public:
	FontMap();
	
	void SetTextureAtlas(TextureAtlas* textureAtlas, const char* texturePrefix);
	
	int m_DataSetId;
	CompiledFont m_Font;
	const Atlas_ImageInfo* m_Texture;
};
