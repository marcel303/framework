#include "ResFont.h"

ResFont::Glyph::Glyph()
{
}

ResFont::Glyph::Glyph(float u1, float v1, float u2, float v2, int width)
{
	Initialize(u1, v1, u2, v2, width);
}

void ResFont::Glyph::Initialize(float u1, float v1, float u2, float v2, int width)
{
	this->u1 = u1;
	this->v1 = v1;
	this->u2 = u2;
	this->v2 = v2;

	this->width = width;
}

int ResFont::Glyph::GetWidth() const
{
	return width;
}

ResFont::ResFont()
	: Res()
{
	SetType(RES_FONT);
}

ResFont::~ResFont()
{
	for (std::map<char, Glyph*>::iterator i = m_glyphs.begin(); i != m_glyphs.end(); ++i)
		delete i->second;
}

ShTex ResFont::GetTexture()
{
	return m_texture;
}

int ResFont::GetHeight() const
{
	return m_height;
}

int ResFont::GetSpacing() const
{
	return m_spacing;
}

const ResFont::Glyph* ResFont::GetGlyph(char character) const
{
	std::map<char, Glyph*>::const_iterator iterator;

	iterator = m_glyphs.find(character);

	if (iterator == m_glyphs.end())
		return 0;
	else
		return iterator->second;
}

void ResFont::SetTexture(ShTex texture)
{
	m_texture = texture;
}

void ResFont::SetHeight(int height)
{
	m_height = height;
}

void ResFont::SetSpacing(int spacing)
{
	m_spacing = spacing;
}

void ResFont::AddGlyph(char character, const Glyph& glyph)
{
	Assert(m_glyphs.find(character) == m_glyphs.end());

	Glyph* temp = new Glyph();

	*temp = glyph;

	m_glyphs[character] = temp;
}

int ResFont::CalculateTextLength(const std::string& text)
{
	int result = 0;

	for (size_t i = 0; i < text.length(); ++i)
	{
		const Glyph* glyph = GetGlyph(text[i]);

		if (glyph)
			result += glyph->width;

		if (i + 1 < text.length())
			result += GetSpacing();
	}

	return result;
}
