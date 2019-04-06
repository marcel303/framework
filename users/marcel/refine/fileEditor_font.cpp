#include "fileEditor_font.h"

FileEditor_Font::FileEditor_Font(const char * in_path)
	: path(in_path)
{
}

void FileEditor_Font::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	if (hasFocus == false)
		return;
	
	clearSurface(0, 0, 0, 0);
	
	setFont(path.c_str());
	pushFontMode(FONT_BITMAP);
	setColor(colorWhite);
	
	const int fontSize = std::max(4, std::min(sx, sy) / 16);
	
	for (int xi = 0; xi < 16; ++xi)
	{
		for (int yi = 0; yi < 8; ++yi)
		{
			const int x = (xi + .5f) * sx / 16;
			const int y = (yi + .5f) * sy / 16 + sy/4;
			
			const char c = xi + yi * 16;
			
			if (isprint(c))
				drawText(x, y, fontSize, 0, -1, " %c", c);
		}
	}
	
	popFontMode();
}
