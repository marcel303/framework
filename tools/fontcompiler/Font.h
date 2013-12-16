#pragma once

#include <string>
#include "Image.h"
#include "Types.h"

class Atlas_ImageInfo;

class FontGlyph
{
public:
	void Setup(std::string shapeName, float sizeX, float sizeY, Image& image);
	
	std::string m_ShapeName;
	float m_SizeX;
	float m_SizeY;
	
	Image m_Image;
	Atlas_ImageInfo* m_AtlasImage;
	Vec2F m_Min;
	Vec2F m_Max;
};

class Font
{
public:
	Font();
	~Font();
	void Initialize();
	void Clear();
	
	FontGlyph* Glyph_get(int index);
	const FontGlyph* Glyph_get(int index) const;
	void Glyph_set(int index, FontGlyph* glyph);
	
	std::string m_TextureFileName;
	int m_SpaceSx;
	
private:
	FontGlyph* m_Glyphs[256];
};
