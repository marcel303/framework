#ifndef RESFONT_H
#define RESFONT_H

#include <map>
#include "Res.h"
//#include "ResourceTexture.h"

class ResFont : public Res
{
public:
	class Glyph
	{
	public:
		Glyph();
		Glyph(float u1, float v1, float u2, float v2, int width);
		void Initialize(float u1, float v1, float u2, float v2, int width);

		int GetWidth() const;

		float u1, v1;
		float u2, v2;

		int width;
	};

	ResFont();
	virtual ~ResFont();

	ShTex GetTexture();
	int GetHeight() const;
	int GetSpacing() const;
	const Glyph* GetGlyph(char character) const;

	void SetTexture(ShTex texture);
	void SetHeight(int height);
	void SetSpacing(int spacing);

	void AddGlyph(char character, const Glyph& glyph);
	int CalculateTextLength(const std::string& text);

private:
	ShTex m_texture;
	int m_height;
	int m_spacing;

	std::map<char, Glyph*> m_glyphs;
};

#endif
