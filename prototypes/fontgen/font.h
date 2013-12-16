#pragma once

class Glyph
{
public:
	Glyph();
	
	char m_Values[5][5];
};

class Font
{
public:
	Font();
	
	Glyph m_Glyphs[256];
	
private:
	void Initialize();
	void SetChar(int index, const char* line1, const char* line2, const char* line3, const char* line4, const char* line5);
};
