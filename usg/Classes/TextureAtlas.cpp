#include "Atlas.h"
#include "Res.h"
#include "TextureAtlas.h"

TextureAtlas::TextureAtlas(Res* texture, const Atlas* atlas)
{
	m_Texture = texture;
	m_Atlas = atlas;
}

TextureAtlas::~TextureAtlas()
{
	m_Texture->Close();
	delete m_Texture;
	m_Texture = 0;

	delete m_Atlas;
	m_Atlas = 0;
}
