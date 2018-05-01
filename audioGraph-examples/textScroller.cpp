/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "FileStream.h"
#include "textScroller.h"
#include "StreamReader.h"
#include "StringEx.h"

TextScroller::TextScroller()
	: lines()
	, progress(0.f)
{
}

void TextScroller::open(const char * filename)
{
	try
	{
		FileStream stream(filename, (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		lines = reader.ReadAllLines();
		for (auto & line : lines)
			line = String::Trim(line);
	}
	catch (std::exception & e)
	{
		logError("failed to read text: %s", e.what());
	}
}

void TextScroller::draw()
{
	const int kLineSpacing = 7;
	const int fontSize = 18;
	
	float sx[1024];
	float sy[1024];
	
	for (size_t i = 0; i < lines.size(); ++i)
	{
		auto & line = lines[i];
		measureTextArea(fontSize, GFX_SX*2/3, sx[i], sy[i], "%s", line.c_str());
	}
	
	int totalSy = 0;
	for (size_t i = 0; i < lines.size(); ++i)
		totalSy += sy[i] + kLineSpacing;
	
	const float scrollY = - progress * totalSy;
	
	gxPushMatrix();
	{
		gxTranslatef(0, scrollY, 0);
		
		const int x1 = 60;
		const int y1 = 200;
		
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			int x = 60;
			int y = y1;
			
			setColor(0, 0, 0, 100);
			for (size_t i = 0; i < lines.size(); ++i)
			{
				auto & line = lines[i];
				
				if (!line.empty())
					hqFillRoundedRect(x - 7, y - 3, x + sx[i] + 7, y + sy[i] + 3, 4.f);
				
				y += sy[i] + kLineSpacing;
			}
		}
		hqEnd();

		setColor(colorWhite);
		beginTextBatch();
		{
			int x = x1;
			int y = y1;
			
			for (size_t i = 0; i < lines.size(); ++i)
			{
				auto & line = lines[i];
				
				const float d = fabsf(y + scrollY - 200.f);
				const float l = powf(fmaxf(0.f, 1.f - d / 600.f), 1.4f);
				
				setLumif(l);
				drawTextArea(x, y, GFX_SX*2/3, fontSize, "%s", line.c_str());
				
				y += sy[i] + kLineSpacing;
			}
		}
		endTextBatch();
	}
	gxPopMatrix();
}
