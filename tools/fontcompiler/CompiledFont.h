#pragma once

#include "FixedSizeString.h"
#include "Stream.h"
#include "Types.h"

class CompiledFont_Glyph
{
public:
	CompiledFont_Glyph();

	void Setup(float width, float height, Vec2F min, Vec2F max);
	
	XBOOL m_IsEmpty;
	float m_Width;
	float m_Height;
	Vec2F m_Min;
	Vec2F m_Max;
};

class CompiledFont
{
public:
	CompiledFont();
	
	float MeasureChar(char c) const;
	float MeasureText(const char* text) const;
	
	void Clear();
	void CalculateHeight();
	
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	// note: this is a hack
	void SpaceSx_set(int sx);

	FixedSizeString<128> m_TextureFileName;
	float m_Height;
	int m_SpaceSx;
	CompiledFont_Glyph m_Glyphs[256];
};
