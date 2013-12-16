#include "Atlas.h"
#include "Debugging.h"
#include "FontMap.h"

FontMap::FontMap()
{
	m_Texture = 0;
}

void FontMap::SetTextureAtlas(TextureAtlas* textureAtlas, const char* texturePrefix)
{
	m_Texture = textureAtlas->m_Atlas->GetImage(m_Font.m_TextureFileName.c_str());
	
	Assert(m_Texture != 0);
}
