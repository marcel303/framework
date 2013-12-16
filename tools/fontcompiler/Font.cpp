#include "Font.h"

void FontGlyph::Setup(std::string shapeName, float sizeX, float sizeY, Image& image)
{
	m_ShapeName = shapeName;
	m_SizeX = sizeX;
	m_SizeY = sizeY;

	m_Image.SetSize(image.m_Sx, image.m_Sy);
			
	image.Blit(&m_Image, 0, 0);
			
	m_AtlasImage = 0;
}

Font::Font()
{
	Initialize();
}

Font::~Font()
{
	Clear();
}

void Font::Initialize()
{
	for (int i = 0; i < 256; ++i)
		m_Glyphs[i] = 0;
}

void Font::Clear()
{
	for (int i = 0; i < 256; ++i)
	{
		delete m_Glyphs[i];
		
		m_Glyphs[i] = 0;
	}
}

FontGlyph* Font::Glyph_get(int index)
{
	return m_Glyphs[index];
}

const FontGlyph* Font::Glyph_get(int index) const
{
	return m_Glyphs[index];
}

void Font::Glyph_set(int index, FontGlyph* glyph)
{
	m_Glyphs[index] = glyph;
}
