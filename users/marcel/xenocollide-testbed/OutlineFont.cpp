/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include "StdAfx.h"

#include "OutlineFont.h"

OutlineFont::OutlineFont( HDC hdc, const char* fontname)
{
	if (!hdc) return;
	if (!fontname) return;

    m_hDC = hdc;
    m_listbase = glGenLists(256);

    LOGFONT logfont;
    GLYPHMETRICSFLOAT glyphMetrics[256];

    logfont.lfHeight        = -12;
    logfont.lfWidth         = 0;
    logfont.lfEscapement    = 0;
    logfont.lfOrientation   = logfont.lfEscapement;
    logfont.lfWeight        = FW_NORMAL;
    logfont.lfItalic        = FALSE;
    logfont.lfUnderline     = FALSE;
    logfont.lfStrikeOut     = FALSE;
    logfont.lfCharSet       = ANSI_CHARSET;
    logfont.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality       = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily = FF_DONTCARE|DEFAULT_PITCH;
    strcpy(logfont.lfFaceName, fontname);

	HFONT hFont = CreateFontIndirect(&logfont);
    BOOL success = (hFont != NULL);
	HFONT oldfont = (HFONT) SelectObject(m_hDC, hFont);
    if (!success || !wglUseFontOutlines( m_hDC, 0, 256, m_listbase, 0.0, 0.0, WGL_FONT_POLYGONS, glyphMetrics ) )
	{
        glDeleteLists(m_listbase, 256);
        m_listbase = 0;
    }
    else
	{
        SelectObject(m_hDC, oldfont);
    }
}

OutlineFont::~OutlineFont()
{
    glDeleteLists(m_listbase, 256);
    m_listbase = 0;
}


void OutlineFont::DrawString(const char* s)
{
	if (!s) return;

	int32 len = (int32) strlen(s);
	if (len < 1) return;

	glPushAttrib(GL_LIST_BIT);
		glListBase(m_listbase);
		glCallLists(len, GL_UNSIGNED_BYTE, (const GLvoid*)s);
	glPopAttrib();
}


