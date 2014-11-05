#ifndef RESLOADERFONT_H
#define RESLOADERFONT_H

#include <string>
#include <vector>
#include "ResLoader.h"

class ResLoaderFont : public ResLoader
{
public:
	virtual Res* Load(const std::string& name);

private:
	class TempGlyph
	{
	public:
		inline void Initialize(char character, int x1, int y1, int x2, int y2, int cellWidth, int cellHeight)
		{
			this->character = character;
			min[0] = x1;
			min[1] = y1;
			max[0] = x2;
			max[1] = y2;

			this->cellWidth = cellWidth;
			this->cellHeight = cellHeight;
		}

		inline bool Empty()
		{
			return min[0] >= max[0] || min[1] >= max[1];
		}

		char character;

		int min[2];
		int max[2];

		int cellWidth;
		int cellHeight;
	};

	bool GenerateGlyphs(const std::string& type, ShTex texture, std::vector<TempGlyph>& out_glyphs);
	bool AutoFitGlyphs(ShTex texture, std::vector<TempGlyph>& out_glyphs);
	bool AutoFitGlyph(ShTex texture, TempGlyph& out_glyph);
};

#endif
