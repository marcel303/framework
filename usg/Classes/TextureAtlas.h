#pragma once

#include "Forward.h"

class TextureAtlas
{
public:
	TextureAtlas(Res* texture, const Atlas* atlas);
	~TextureAtlas();
	
	Res* m_Texture;
	const Atlas* m_Atlas;
};

class AtlasImageMap
{
public:
	Res* m_TextureAtlas;
	const Atlas_ImageInfo* m_Info;
};
