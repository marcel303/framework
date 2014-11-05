#include "Archive.h"
#include "ResFont.h"
#include "ResLoaderFont.h"
#include "ResMgr.h"
#include "ResTex.h"

Res* ResLoaderFont::Load(const std::string& name)
{
	Ar registry;

	if (!registry.Read(name))
	{
		FASSERT(0);
		return 0;
	}

	ResFont* font = new ResFont();

	std::string in_type = registry("type", "ASCII 16x16");
	std::string in_texture = registry("texture", "");
	int in_spacing = registry("spacing", 0);
	int in_autofit = registry("autofit", 1);
	int in_border = registry("border", 0);

	// Check input parameters.

	if (in_texture == "")
	{
		FASSERT(0);
		return 0;
	}

	ShTex texture = RESMGR.GetTex(in_texture, true);
	
	std::vector<TempGlyph> glyphs;

	if (!GenerateGlyphs(in_type, texture, glyphs))
	{
		FASSERT(0);
		delete font;
		return 0;
	}

	if (in_autofit)
	{
		if (!AutoFitGlyphs(texture, glyphs))
		{
			FASSERT(0);
			delete font;
			return 0;
		}
	}

	int height = 0;

	// Add all non-empty glyphs to the font.
	for (size_t i = 0; i < glyphs.size(); ++i)
	{
		if (!glyphs[i].Empty())
		{
			int width;
			float u1, v1, u2, v2;

			width = glyphs[i].max[0] - glyphs[i].min[0] + 1;
			height = glyphs[i].cellHeight;

			const size_t textureW = texture->GetW();
			const size_t textureH = texture->GetH();

			u1 = (glyphs[i].min[0] - in_border) / float(textureW);
			v1 = (glyphs[i].min[1] - in_border) / float(textureH);
			u2 = (glyphs[i].max[0] + 1 + in_border) / float(textureW);
			v2 = (glyphs[i].max[1] + 1 + in_border) / float(textureH);

			ResFont::Glyph glyph;

			glyph.Initialize(u1, v1, u2, v2, width);

			font->AddGlyph(glyphs[i].character, glyph);
		}
	}

	font->SetHeight(height);
	font->SetSpacing(in_spacing);
	font->SetTexture(texture);

	return font;
}

bool ResLoaderFont::GenerateGlyphs(const std::string& type, ShTex texture, std::vector<TempGlyph>& out_glyphs)
{
	bool result = true;

	const int textureW = static_cast<int>(texture->GetW());
	const int textureH = static_cast<int>(texture->GetH());

	if (type == "ASCII 16x16")
	{
		// Generate 16x16 glyphs.
		for (int x = 0; x < 16; ++x)
		{
			for (int y = 0; y < 16; ++y)
			{
				int x1, y1, x2, y2;

				x1 = textureW / 16 * x;
				y1 = textureH / 16 * y;
				x2 = textureW / 16 * (x + 1) - 1;
				y2 = textureH / 16 * (y + 1) - 1;

				TempGlyph glyph;

				glyph.Initialize(x + y * 16, x1, y1, x2, y2, textureW / 16, textureH / 16);

				out_glyphs.push_back(glyph);
			}
		}
	}
	else
	{
		return result = false;
	}

	return result;
}

bool ResLoaderFont::AutoFitGlyphs(ShTex texture, std::vector<TempGlyph>& out_glyphs)
{
	bool result = true;

	for (size_t i = 0; i < out_glyphs.size(); ++i)
	{
		TempGlyph& glyph = out_glyphs[i];

		if (!AutoFitGlyph(texture, glyph))
			result = false;
	}

	return result;
}

bool ResLoaderFont::AutoFitGlyph(ShTex texture, TempGlyph& out_glyph)
{
	bool result = true;

	if (out_glyph.character == ' ')
	{
		int width = out_glyph.max[0] - out_glyph.min[0] + 1;
		out_glyph.max[0] = out_glyph.min[0] + width / 3;

		return result;
	}

	int min = out_glyph.max[0];
	int max = out_glyph.min[0];

	for (int x = out_glyph.min[0]; x <= out_glyph.max[0]; ++x)
	{
		for (int y = out_glyph.min[1]; y <= out_glyph.max[1]; ++y)
		{
			Color color = texture->GetPixel(x, y);
			if ((color.m_rgba[0] != 0 || color.m_rgba[1] != 0 || color.m_rgba[2] != 0) && color.m_rgba[3] != 0.0f)
			{
				if (x < min)
					min = x;
				if (x > max)
					max = x;
			}
		}
	}

	out_glyph.min[0] = min;
	out_glyph.max[0] = max;

	return result;
}
