#include <stdio.h>
#include <string>
#include "Font.h"

/**

Font -> vector graphic generator

Thanks, 'Ferry'..

**/

enum DrawMode
{
	DrawMode_Circles,
	DrawMode_HLines,
	DrawMode_VLines,
	DrawMode_Filled
};

int main(int argc, char* argv[])
{
	Font font;
	
	int num = 0;
	int offsetX = 0;
	int offsetY = 0;
	
	for (int i = 0; i < 256; ++i)
	{
		char fileName[64];
		
		sprintf(fileName, "glyph%d.vg", i);
		
		FILE* file = fopen(fileName, "wt");
		
		DrawMode mode = DrawMode_HLines;
		int scale = 5;
		bool empty = true;
		
		switch (mode)
		{
			case DrawMode_Circles:
			{
				for (int y = 0; y < 5; ++y)
				{
					for (int x = 0; x < 5; ++x)
					{
						if (!font.m_Glyphs[i].m_Values[y][x])
							continue;
						
						empty = false;
						
						fprintf(file, "circle\n");
						fprintf(file, "\tposition %d %d\n", x * scale, y * scale);
						fprintf(file, "\tradius %d\n", scale / 3);
						fprintf(file, "\tstroke 1\n");
						fprintf(file, "\tfilled 0\n");
						fprintf(file, "\thardness 1.0\n");
					}
				}
				break;
			}
			case DrawMode_HLines:
			{
				for (int y = 0; y < 5; ++y)
				{
					int x1 = -1;
					int x2 = -1;
					
					for (int x = 0; x < 5; ++x)
					{
						bool emit = false;
						
						if (!font.m_Glyphs[i].m_Values[y][x])
						{
							if (x1 != -1)
								emit = true;
						}
						else
						{
							if (x1 == -1)
								x1 = x;
								
							x2 = x;
						}
						
						if (x == 4 && x1 != -1)
						{
							emit = true;
						}
						
						if (emit)
						{
							empty = false;
							
							fprintf(file, "poly\n");
							fprintf(file, "\tpoint %d %d\n", offsetX + x1 * scale, offsetY + y * scale);
							fprintf(file, "\tpoint %d %d\n", offsetX + (x2 + 1) * scale, offsetY + y * scale);
							fprintf(file, "\tstroke 1.0\n");
							fprintf(file, "\tfilled 0\n");
							fprintf(file, "\thardness 100.0\n");
							
							x1 = -1;
						}
					}
				}
				break;
			}
			
			case DrawMode_VLines:
			{
				break;
			}
			
			case DrawMode_Filled:
			{
				for (int y = 0; y < 5; ++y)
				{
					for (int x = 0; x < 5; ++x)
					{
						if (!font.m_Glyphs[i].m_Values[y][x])
							continue;
						
						empty = false;
						
						int x1 = (x + 0) * scale - 0;
						int x2 = (x + 1) * scale - 0;
						int y1 = (y + 0) * scale - 0;
						int y2 = (y + 1) * scale - 0;
						
						fprintf(file, "poly\n");
						fprintf(file, "\tpoint %d %d\n", offsetX + x1, offsetY + y1);
						fprintf(file, "\tpoint %d %d\n", offsetX + x2, offsetY + y1);
						fprintf(file, "\tpoint %d %d\n", offsetX + x2, offsetY + y2);
						fprintf(file, "\tpoint %d %d\n", offsetX + x1, offsetY + y2);
						fprintf(file, "\tstroke 1.0\n");
						fprintf(file, "\tfilled 1\n");
						fprintf(file, "\tfill 255\n");
						fprintf(file, "\thardness 1.0\n");
					}
				}
				break;
			}
		}
		
		if (!empty)
		{
			num++;
			
			int spacing = scale * 2 * 5;
			
			int row = num / 16;
			int col = num - row * 16;

			offsetX = col * spacing;
			offsetY = row * spacing;
		}
		
		fclose(file);
	}
	
	return 0;
}