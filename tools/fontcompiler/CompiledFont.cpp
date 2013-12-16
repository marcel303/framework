#include "CompiledFont.h"
#include "StreamReader.h"
#include "StreamWriter.h"

CompiledFont_Glyph::CompiledFont_Glyph()
{
	m_IsEmpty = XTRUE;
	m_Width = 0.0f;
	m_Height = 0.0f;
}

void CompiledFont_Glyph::Setup(float width, float height, Vec2F min, Vec2F max)
{
	m_IsEmpty = XFALSE;
	
	m_Width = width;
	m_Height = height;
	m_Min = min;
	m_Max = max;
}

//

CompiledFont::CompiledFont()
{
	m_Height = 0.0f;
	m_SpaceSx = 0;
}

float CompiledFont::MeasureChar(char c) const
{
	return m_Glyphs[(int)c].m_Width;	
}

float CompiledFont::MeasureText(const char* text) const
{
	float result = 0.0f;
	
	for (int i = 0; text[i]; ++i)
		result += MeasureChar(text[i]);
	
	return result;
}

void CompiledFont::Clear()
{
	for (int i = 0; i < 256; ++i)
		m_Glyphs[i].m_IsEmpty = XTRUE;
}

void CompiledFont::CalculateHeight()
{
	float height = 0.0f;
	
	for (int i = 0; i < 256; ++i)
	{
		CompiledFont_Glyph& glyph = m_Glyphs[i];
		
		if (glyph.m_IsEmpty)
			continue;
		
		if (glyph.m_Height > height)
			height = glyph.m_Height;
	}
	
	m_Height = height;
}

void CompiledFont::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	Clear();
	
	m_TextureFileName = reader.ReadText_Binary().c_str();
	
	m_Height = reader.ReadFloat();
	
	m_SpaceSx = reader.ReadInt32();
	
	int count = reader.ReadInt32();
	
	for (int i = 0; i < count; ++i)
	{
		int index = reader.ReadInt32();
		
		CompiledFont_Glyph& glyph = m_Glyphs[index];
		
		glyph.m_IsEmpty = XFALSE;
		glyph.m_Width = reader.ReadFloat();
		glyph.m_Height = m_Height;
		glyph.m_Min[0] = reader.ReadFloat();
		glyph.m_Min[1] = reader.ReadFloat();
		glyph.m_Max[0] = reader.ReadFloat();
		glyph.m_Max[1] = reader.ReadFloat();
	}
	
	//m_Glyphs[(int)' '].m_Width = 10.0f;
	m_Glyphs[(int)' '].m_Width = (float)m_SpaceSx;
}

void CompiledFont::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	writer.WriteText_Binary(m_TextureFileName.c_str());
	
	writer.WriteFloat(m_Height);
	
	writer.WriteInt32(m_SpaceSx);
	
	int count = 0;
	
	for (int i = 0; i < 256; ++i)
		if (!m_Glyphs[i].m_IsEmpty)
			count++;

	writer.WriteInt32(count);
	
	for (int i = 0; i < 256; ++i)
	{
		CompiledFont_Glyph& glyph = m_Glyphs[i];
		
		if (glyph.m_IsEmpty)
			continue;
		
		writer.WriteInt32(i);
		
		writer.WriteFloat(glyph.m_Width);
		//LOG_DBG("glyph sx: %03.2f", glyph.m_Width);
		writer.WriteFloat(glyph.m_Min[0]);
		writer.WriteFloat(glyph.m_Min[1]);
		writer.WriteFloat(glyph.m_Max[0]);
		writer.WriteFloat(glyph.m_Max[1]);
	}
}

void CompiledFont::SpaceSx_set(int sx)
{
	m_SpaceSx = sx;
	
	m_Glyphs[(int)' ' ].m_Width = (float)sx;
}
