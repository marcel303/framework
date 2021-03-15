/*
	Copyright (C) 2020 Marcel Smit
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

#include <GL/glew.h> // glReadPixels
#include "framework.h"
#include "gfx-framework.h"
#include "jsusfx.h"
#include "StringEx.h"

#include "WDL/wdlstring.h"

#include "data/lice-gradient.ps"
#include "data/lice-gradient.vs"

//#define STUB logDebug("function %s not implemented", __FUNCTION__)
#define STUB do { } while (false)

//

JsusFx_Image::~JsusFx_Image()
{
	free();
}

void JsusFx_Image::free()
{
	if (surface != nullptr)
	{
		delete surface;
		surface = nullptr;
	}
	
	isValid = false;
}

void JsusFx_Image::resize(const int sx, const int sy)
{
	if (sx <= 0 || sy <= 0)
	{
		isValid = false;
	}
	else
	{
		if (surface == nullptr || surface->getWidth() != sx || surface->getHeight() != sy)
		{
			if (surface != nullptr)
			{
				//logDebug("image resize: (%d, %d) -> (%d, %d)", surface->getWidth(), surface->getHeight(), sx, sy);
				
				delete surface;
				surface = nullptr;
			}
			
			surface = new Surface(sx, sy, false, false, false);
		}
		
		surface->clear();
			
		isValid = true;
	}
}

//

JsusFx_ImageCache::JsusFx_ImageCache()
{
	dummyImage.resize(1, 1);
}

void JsusFx_ImageCache::free()
{
	for (int i = 0; i < kMaxImages; ++i)
	{
		images[i].free();
	}
}

int JsusFx_ImageCache::alloc(const int sx, const int sy)
{
	Assert(sx > 0 && sy > 0);
	
	for (int i = 0; i < kMaxImages; ++i)
	{
		if (images[i].surface == nullptr)
		{
			images[i].resize(sx, sy);
			return i;
		}
	}
	
	return -2;
}

JsusFx_Image & JsusFx_ImageCache::get(const int index)
{
	Assert(index != -1);
	
	if (index < 0 || index >= kMaxImages)
		return dummyImage;
	else
		return images[index];
}

//

#define IMAGE_SCOPE updateSurface()

#define BLEND_SCOPE updateBlendMode()

#define LINE_STROKE 1.f

#define PRIM_SCOPE(type) updatePrimType(type)

JsusFxGfx_Framework::JsusFxGfx_Framework(JsusFx & _jsusFx)
	: JsusFxGfx()
	, jsusFx(_jsusFx)
	, surface(nullptr)
	, drawTransform(true)
{
	shaderSource("lice-gradient.ps", s_liceGradientPs);
	shaderSource("lice-gradient.vs", s_liceGradientVs);
}

void JsusFxGfx_Framework::setup(
	Surface * _surface,
	const int w,
	const int h,
	const int _mouseX,
	const int _mouseY,
	const bool _inputEnabled)
{
	surface = _surface;
	mouseX = _mouseX;
	mouseY = _mouseY;
	inputEnabled = _inputEnabled;
	
	setup(w, h);
}

void JsusFxGfx_Framework::setup(const int w, const int h)
{
	// update gfx state
	
	*m_gfx_w = w;
	*m_gfx_h = h;
	
	if (*m_gfx_clear > -1.0)
	{
		const int a = (int)*m_gfx_clear;
		
		const int r = (a >>  0) & 0xff;
		const int g = (a >>  8) & 0xff;
		const int b = (a >> 16) & 0xff;
		
		if (surface != nullptr)
		{
			surface->clear(r, g, b, 0);
		}
		else
		{
			setColorf(r, g, b, 0.f);
			pushBlend(BLEND_OPAQUE);
			drawRect(0, 0, w, h);
			popBlend();
		}
	}
	
	*m_gfx_texth = m_fontSize;
	
	*m_gfx_dest = -1.0;
	*m_gfx_a = 1.0;
	
	// update mouse state
	
	*m_mouse_x = mouseX;
	*m_mouse_y = mouseY;
	
	/*
	mouse_cap is a bitfield of mouse and keyboard modifier state.
		1: left mouse button
		2: right mouse button
		4: Control key (Windows), Command key (OSX)
		8: Shift key
		16: Alt key (Windows), Option key (OSX)
		32: Windows key (Windows), Control key (OSX) -- REAPER 4.60+
		64: middle mouse button -- REAPER 4.60+
	*/
	
	int vflags = 0;
	
	if (inputEnabled)
	{
		const bool isInside =
			mouseX >= 0 && mouseX < *m_gfx_w &&
			mouseY >= 0 && mouseY < *m_gfx_h;
		
		if (isInside)
		{
			if (mouse.wentDown(BUTTON_LEFT))
				mouseFlags |= 0x1;
			if (mouse.wentDown(BUTTON_RIGHT))
				mouseFlags |= 0x2;
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
			mouseFlags &= ~0x1;
		if (mouse.wentUp(BUTTON_RIGHT))
			mouseFlags &= ~0x2;

		if (mouseFlags & 0x1)
			vflags |= 0x1;
		if (mouseFlags & 0x2)
			vflags |= 0x2;
		
	#if defined(MACOS)
		if (keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI))
			vflags |= 0x4;
	#else
		if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL))
			vflags |= 0x4;
	#endif

		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
			vflags |= 0x8;
		if (keyboard.isDown(SDLK_LALT) || keyboard.isDown(SDLK_RALT))
			vflags |= 0x10;
		
	#if defined(MACOS)
		if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL))
			vflags |= 0x20;
	#else
		if (keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI))
			vflags |= 0x20;
	#endif
	
		// note : 0x40 would be middle mouse button
	}
	
	*m_mouse_cap = vflags;
	
	*m_mouse_wheel = mouse.scrollY;
	
	lastKey = 0;
	
	for (auto & e : keyboard.events)
	{
		if (e.type == SDL_KEYDOWN)
		{
			lastKey = e.key.keysym.sym;
		}
	}
}

void JsusFxGfx_Framework::beginDraw()
{
	Assert(primType == kPrimType_Other);
	
	pushSurface(surface);
	gxLoadMatrixf(drawTransform.m_v);
	currentImageIndex = -1;

	pushBlend(BLEND_OPAQUE);
	currentBlendMode = -1;
	updateBlendMode();
}

void JsusFxGfx_Framework::endDraw()
{
	PRIM_SCOPE(kPrimType_Other);
	
	popBlend();
	currentBlendMode = -1;
	
	popSurface();
	currentImageIndex = -2;
	
// todo : reset surface or not ?
	//surface = nullptr;
}

void JsusFxGfx_Framework::updatePrimType(PrimType _primType)
{
	if (_primType != primType)
	{
		if (primType == kPrimType_Rect)
			gxEnd();
		else if (primType == kPrimType_RectLine)
			gxEnd();
		else if (primType == kPrimType_HqLine)
			hqEnd();
		else if (primType == kPrimType_HqFillCircle)
			hqEnd();
		else if (primType == kPrimType_HqStrokeCircle)
			hqEnd();
		
		primType = _primType;
		
		if (primType == kPrimType_Rect)
			gxBegin(GX_QUADS);
		else if (primType == kPrimType_RectLine)
			gxBegin(GX_LINES);
		else if (primType == kPrimType_HqLine)
			hqBegin(HQ_LINES);
		else if (primType == kPrimType_HqFillCircle)
			hqBegin(HQ_FILLED_CIRCLES);
		else if (primType == kPrimType_HqStrokeCircle)
			hqBegin(HQ_STROKED_CIRCLES);
	}
}

void JsusFxGfx_Framework::updateBlendMode()
{
	const int mode = *m_gfx_mode;
	
	if (mode != currentBlendMode)
	{
		PRIM_SCOPE(kPrimType_Other);
		
		currentBlendMode = mode;
		
		if (mode & 0x1)
			setBlend(BLEND_ADD);
		else
			setBlend(BLEND_ALPHA);
	}
}

void JsusFxGfx_Framework::updateColor()
{
	setColorf(*m_gfx_r, *m_gfx_g, *m_gfx_b, *m_gfx_a);
}

void JsusFxGfx_Framework::updateSurface()
{
	const int index = *m_gfx_dest;
	Assert(index >= -1);
	
	if (index != currentImageIndex)
	{
		PRIM_SCOPE(kPrimType_Other);
		
		popSurface();
		
		if (index == -1)
		{
			pushSurface(surface);
			gxLoadMatrixf(drawTransform.m_v);
		}
		else
		{
			auto image = &imageCache.get(index);
			
			Assert(image->isValid);
			pushSurface(image->surface);
		}
		
		currentImageIndex = index;
	}
}

void JsusFxGfx_Framework::gfx_line(int np, EEL_F ** params)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_HqLine);
	
	const int x1 = (int)floor(params[0][0]);
	const int y1 = (int)floor(params[1][0]);
	const int x2 = (int)floor(params[2][0]);
	const int y2 = (int)floor(params[3][0]);
	
	updateColor();
	hqLine(x1, y1, LINE_STROKE, x2, y2, LINE_STROKE);
}

void JsusFxGfx_Framework::gfx_rect(int np, EEL_F ** params)
{
	const int x1 = (int)floor(params[0][0]);
	const int y1 = (int)floor(params[1][0]);
	const int w = (int)floor(params[2][0]);
	const int h = (int)floor(params[3][0]);
	const int x2 = x1 + w;
	const int y2 = y1 + h;
	
	const bool filled = (np < 5 || params[4][0] > 0.5);
	
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(filled ? kPrimType_Rect : kPrimType_RectLine);
	
	if (filled)
	{
		updateColor();
		
		gxVertex2f(x1, y1);
		gxVertex2f(x2, y1);
		gxVertex2f(x2, y2);
		gxVertex2f(x1, y2);
	}
	else
	{
		updateColor();
		
		gxVertex2f(x1, y1);
		gxVertex2f(x2, y1);
		
		gxVertex2f(x2, y1);
		gxVertex2f(x2, y2);
		
		gxVertex2f(x2, y2);
		gxVertex2f(x1, y2);
		
		gxVertex2f(x1, y2);
		gxVertex2f(x1, y1);
	}
}

void JsusFxGfx_Framework::gfx_circle(EEL_F x, EEL_F y, EEL_F radius, bool fill, bool aa)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(fill ? kPrimType_HqFillCircle : kPrimType_HqStrokeCircle);
	
	if (fill)
	{
		updateColor();
		hqFillCircle(x, y, radius);
	}
	else
	{
		updateColor();
		hqStrokeCircle(x, y, radius, 1.f);
	}
}

void JsusFxGfx_Framework::gfx_triangle(EEL_F ** parms, int np)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	if (np >= 6)
	{
		np &= ~1;
		
		if (np == 6)
		{
			const int x1 = (int)parms[0][0];
			const int y1 = (int)parms[1][0];
			const int x2 = (int)parms[2][0];
			const int y2 = (int)parms[3][0];
			const int x3 = (int)parms[4][0];
			const int y3 = (int)parms[5][0];
			
			updateColor();
			
			hqBegin(HQ_FILLED_TRIANGLES);
			hqFillTriangle(x1, y1, x2, y2, x3, y3);
			hqEnd();
		}
		else
		{
			// convex polygon
			
			const int x1 = (int)parms[0][0];
			const int y1 = (int)parms[1][0];
			const int x2 = (int)parms[2][0];
			const int y2 = (int)parms[3][0];
			
			updateColor();
			
			hqBegin(HQ_FILLED_TRIANGLES);
			{
				for (int i = 2; i < np / 2; ++i)
				{
					const int x3 = (int)parms[i * 2 + 0][0];
					const int y3 = (int)parms[i * 2 + 1][0];
					
					hqFillTriangle(x1, y1, x2, y2, x3, y3);
				}
			}
			hqEnd();
		}
	}
}

void JsusFxGfx_Framework::gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F useaa)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_HqLine);
	
	updateColor();
	hqLine(*m_gfx_x, *m_gfx_y, LINE_STROKE, xpos, ypos, LINE_STROKE);
	
	*m_gfx_x = xpos;
	*m_gfx_y = ypos;
}

void JsusFxGfx_Framework::gfx_rectto(EEL_F xpos, EEL_F ypos)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	updateColor();
	drawRect(*m_gfx_x, *m_gfx_y, xpos, ypos);
	
	*m_gfx_x = xpos;
	*m_gfx_y = ypos;
}

void JsusFxGfx_Framework::gfx_arc(int np, EEL_F ** parms)
{
	STUB;
	
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	updateColor();
#if 1
	hqBegin(HQ_STROKED_CIRCLES);
	hqStrokeCircle(*parms[0], *parms[1], *parms[2], 1.f);
	hqEnd();
#else
	hqBegin(HQ_FILLED_CIRCLES);
	hqFillCircle(*parms[0], *parms[1], *parms[2]);
	hqEnd();
#endif
}

void JsusFxGfx_Framework::gfx_set(int np, EEL_F ** params)
{
	if (np < 1) return;

	if (m_gfx_r) *m_gfx_r = params[0][0];
	if (m_gfx_g) *m_gfx_g = np > 1 ? params[1][0] : params[0][0];
	if (m_gfx_b) *m_gfx_b = np > 2 ? params[2][0] : params[0][0];
	if (m_gfx_a) *m_gfx_a = np > 3 ? params[3][0] : 1.0;
	if (m_gfx_mode) *m_gfx_mode = np > 4 ? params[4][0] : 0;
	if (np > 5 && m_gfx_dest) *m_gfx_dest = params[5][0];
}

void JsusFxGfx_Framework::gfx_roundrect(int np, EEL_F ** parms)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);

	//const bool aa = np <= 5 || parms[5][0] > .5f;

	const float sx = parms[2][0];
	const float sy = parms[3][0];

	if (sx > 0 && sy > 0)
	{
		const float x = parms[0][0];
		const float y = parms[1][0];

		const float radius = parms[4][0];

		updateColor();
		
		hqBegin(HQ_STROKED_ROUNDED_RECTS);
		hqStrokeRoundedRect(x, y, x + sx, y + sy, radius, LINE_STROKE * 2.f);
		hqEnd();
	}
}

void JsusFxGfx_Framework::gfx_grad_or_muladd_rect(int mode, int np, EEL_F ** parms)
{
	const int x1 = (int)floor(parms[0][0]);
	const int y1 = (int)floor(parms[1][0]);
	const int w = (int)floor(parms[2][0]);
	const int h = (int)floor(parms[3][0]);
	
	if (w <= 0 || h <= 0)
		return;
	
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	if (mode == 0)
	{
		const float r = (float)parms[4][0];
		const float g = (float)parms[5][0];
		const float b = (float)parms[6][0];
		const float a = (float)parms[7][0];
		
		const float xr = np >  8 ? (float)parms[ 8][0] : 0.0f;
		const float xg = np >  9 ? (float)parms[ 9][0] : 0.0f;
		const float xb = np > 10 ? (float)parms[10][0] : 0.0f;
		const float xa = np > 11 ? (float)parms[11][0] : 0.0f;
		
		const float yr = np > 12 ? (float)parms[12][0] : 0.0f;
		const float yg = np > 13 ? (float)parms[13][0] : 0.0f;
		const float yb = np > 14 ? (float)parms[14][0] : 0.0f;
		const float ya = np > 15 ? (float)parms[15][0] : 0.0f;
		
		Shader shader("lice-gradient");
		setShader(shader);
		{
			shader.setImmediate("startPosition", x1, y1);
			shader.setImmediate("startColor", r, g, b, a);
			shader.setImmediate("gradientX", xr, xg, xb, xa);
			shader.setImmediate("gradientY", yr, yg, yb, ya);
			
			drawRect(x1, y1, x1 + w, y1 + h);
		}
		clearShader();
	}
	else
	{
		logDebug("multiply-add rect!");
		
		STUB;
	}
}

void JsusFxGfx_Framework::gfx_drawnumber(EEL_F n, int nd)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	char formatString[32];
	sprintf_s(formatString, sizeof(formatString), "%%.%df", nd);
	
	float sx;
	float sy;
	measureText(m_fontSize, sx, sy, formatString, n);
	
	updateColor();
	drawText(*m_gfx_x, *m_gfx_y + m_fontSize, m_fontSize, +1, -1, formatString, n);
	
	*m_gfx_x += sx;
}

void JsusFxGfx_Framework::gfx_drawchar(EEL_F n)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	const char c = (char)n;
	
	if (c == ' ')
	{
		*m_gfx_x += m_fontSize/4.f;
		return;
	}
	
	float sx;
	float sy;
	measureText(m_fontSize, sx, sy, "%c", c);
	
	updateColor();
	drawText(*m_gfx_x, *m_gfx_y + m_fontSize, m_fontSize, +1, -1, "%c", c);
	
	*m_gfx_x += sx;
}

void JsusFxGfx_Framework::gfx_drawstr(void * opaque, EEL_F ** parms, int nparms, int formatmode)
{
	// mode=1 for format, 2 for purely measure no format, 3 for measure char

	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	int nfmtparms = nparms - 1;
	
	EEL_F ** fmtparms = parms + 1;
	
#ifdef EEL_STRING_DEBUGOUT
	const char * funcname =
		  formatmode == 1 ? "gfx_printf"
		: formatmode == 2 ? "gfx_measurestr"
		: formatmode == 3 ? "gfx_measurechar"
		: "gfx_drawstr";
#endif

	char buf[4096];

	const char * s = nullptr;
	int s_len = 0;
	
	char temp[32];
	if (formatmode == 3)
	{
		temp[0] = (int)parms[0][0];
		temp[1] = 0;
		s_len = 1;
		s = temp;
	}
	else
	{
		const int index = parms[0][0] + .5f;
		
		WDL_FastString *fs = nullptr;
		s = jsusFx.getString(index, &fs);
		
	#ifdef EEL_STRING_DEBUGOUT
		if (!s)
			EEL_STRING_DEBUGOUT("gfx_%s: invalid string identifier %f",funcname,parms[0][0]);
	#endif
	
		if (!s)
		{
			s="<bad string>";
			s_len = 12;
		}
		else if (formatmode == 1)
		{
			extern int eel_format_strings(void *, const char *s, const char *ep, char *, int, int, EEL_F **);
			
			s_len = eel_format_strings(opaque,s,fs?(s+fs->GetLength()):NULL,buf,sizeof(buf),nfmtparms,fmtparms);
			if (s_len < 1)
				return;
			s = buf;
		}
		else
		{
			s_len = fs ? fs->GetLength() : (int)strlen(s);
		}
	}

	if (s_len)
	{
		float sx;
		float sy;
		measureText(m_fontSize, sx, sy, "%s", s);
		
		if (formatmode >= 2)
		{
			if (nfmtparms == 2)
			{
				fmtparms[0][0] = sx;
				fmtparms[1][0] = sy;
			}
		}
		else
		{
			updateColor();
			drawText(*m_gfx_x, *m_gfx_y + m_fontSize - 2, m_fontSize, +1, -1, "%s", s);
			*m_gfx_x += sx;
		}
	}
}

void JsusFxGfx_Framework::gfx_setpixel(EEL_F r, EEL_F g, EEL_F b)
{
	IMAGE_SCOPE;
	BLEND_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	setColorf(r, g, b, *m_gfx_a);
	drawPoint(*m_gfx_x, *m_gfx_y);
}

void JsusFxGfx_Framework::gfx_getpixel(EEL_F * r, EEL_F * g, EEL_F * b)
{
	IMAGE_SCOPE;
	
	const int x = *m_gfx_x;
	const int y = *m_gfx_y;
	
	float rgba[4] = { };
	
#if ENABLE_OPENGL
	GLint restorePackAlignment;
	glGetIntegerv(GL_PACK_ALIGNMENT, &restorePackAlignment);
	checkErrorGL();
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	checkErrorGL();
	
	const int backingScale = framework.getCurrentBackingScale();
	
	glReadPixels(
		x * backingScale, y * backingScale,
		1, 1,
		GL_RGBA, GL_FLOAT,
		rgba);
	checkErrorGL();
	
	glPixelStorei(GL_PACK_ALIGNMENT, restorePackAlignment);
	checkErrorGL();
#else
	//AssertMsg(false, "gfx_getpixel: not implemented for current graphics api");
	
	rgba[0] = 255;
	rgba[1] = 0;
	rgba[2] = 255;
#endif
	
	*r = rgba[0];
	*g = rgba[1];
	*b = rgba[2];
}

EEL_F JsusFxGfx_Framework::gfx_loadimg(JsusFx & jsusFx, int index, EEL_F loadFrom)
{
	PRIM_SCOPE(kPrimType_Other);
	
	const int fileIndex = (int)loadFrom;
	
	if (fileIndex < 0 || fileIndex >= JsusFx::kMaxFileInfos)
		return -1;

	if (jsusFx.fileInfos[fileIndex].isValid() == false)
		return -1;
	
	if (index >= 0 && index < imageCache.kMaxImages)
	{
		JsusFx_Image & image = imageCache.images[index];
		
		const char * filename = jsusFx.fileInfos[fileIndex].filename.c_str();
		
		const GxTextureId texture = getTexture(filename);
		
		if (texture == 0)
		{
			logError("failed to load image %d:%s", index, filename);
			
			return -1;
		}
		else
		{
			int sx;
			int sy;
			gxGetTextureSize(texture, sx, sy);
			
			//
			
			image.resize(sx, sy);
			
			pushSurface(image.surface);
			{
				gxSetTexture(texture);
				setColor(colorWhite);
				pushBlend(BLEND_OPAQUE);
				drawRect(0, 0, sx, sy);
				popBlend();
				gxSetTexture(0);
			}
			popSurface();
			
			return index;
		}
	}

	return -1;
}

void JsusFxGfx_Framework::gfx_getimgdim(EEL_F _img, EEL_F * w, EEL_F * h)
{
	int img = (int)_img;
	
	if (img == -1)
	{
		*w = *m_gfx_w;
		*h = *m_gfx_h;
	}
	else if (img >= 0 && img < imageCache.kMaxImages)
	{
		auto & image = imageCache.images[img];
		
		if (image.isValid)
		{
			*w = image.surface->getWidth();
			*h = image.surface->getHeight();
		}
		else
		{
			*w = 0;
			*h = 0;
		}
	}
	else
	{
		*w = 0;
		*h = 0;
	}
}

EEL_F JsusFxGfx_Framework::gfx_setimgdim(int img, EEL_F * w, EEL_F * h)
{
	if (img >= 0 && img < imageCache.kMaxImages)
	{
		auto & image = imageCache.images[img];
		
		int use_w = (int)*w;
		int use_h = (int)*h;
		if (use_w < 1 || use_h < 1)
			use_w = use_h = 0;
		
		if (use_w > 2048)
			use_w = 2048;
		if (use_h > 2048)
			use_h = 2048;
		
		//logDebug("resizing image %d to (%d, %d)", img, use_w, use_h);
		
		image.resize(use_w, use_h);
	}
	
 	return 1.f;
}

EEL_F JsusFxGfx_Framework::gfx_setfont(int np, EEL_F ** parms)
{
	const int size = np > 2 ? (int)parms[2][0] : 10;
	
	// note : this implementation is very basic and doesn't support selecting a different font face
	
	m_fontSize = size;
	
	return 0.f;
}

EEL_F JsusFxGfx_Framework::gfx_getfont(int np, EEL_F ** parms)
{
	STUB;
	
	return 0.f;
}

EEL_F JsusFxGfx_Framework::gfx_showmenu(EEL_F ** parms, int nparms)
{
	STUB;
	
	return 0.f;
}

EEL_F JsusFxGfx_Framework::gfx_setcursor(EEL_F ** parms, int nparms)
{
#if 1
	const int cursorResId = (int)parms[0][0];
	(void)cursorResId;
	
	char cursorName[64];
	cursorName[0]= 0;
	
	if (nparms > 1)
	{
		WDL_FastString * fs = nullptr;
		const char * p = jsusFx.getString(parms[1][0], &fs);
		
		if (p && p[0])
		{
			strcpy_s(cursorName, sizeof(cursorName), p);
			
			// todo : do something with this cursor ?
		}
	}
	
	return 1;
#else
	return 0;
#endif
}

void JsusFxGfx_Framework::gfx_blurto(EEL_F x, EEL_F y)
{
	STUB;
	
	*m_gfx_x = x;
	*m_gfx_y = y;
}

void JsusFxGfx_Framework::gfx_blit(EEL_F _img, EEL_F scale, EEL_F rotate)
{
	int img = (int)_img;
	
	Assert(img != -1);
	
	if (img >= 0 && img < imageCache.kMaxImages)
	{
		const JsusFx_Image & image = imageCache.images[img];
		
		//Assert(image.isValid); // todo : use a separate ScriptAssert
		
		if (image.isValid)
		{
			IMAGE_SCOPE;
			
			PRIM_SCOPE(kPrimType_Other);
			
			// mode & 0x1 = additive
			// mode & 0x2 = disable source alpha
			// mode & 0x4 = disable filtering
			
			const int mode = *m_gfx_mode;
			
			Assert(img != (int)*m_gfx_dest);
			
			setColorf(1.f, 1.f, 1.f, *m_gfx_a); // pretty sure this is correct
			
			if (mode & 0x1)
				pushBlend(BLEND_ADD);
			else
				pushBlend(BLEND_ALPHA);
			
			if (mode & 0x2)
				pushColorPost(POST_SET_ALPHA_TO_ONE);
			else
				pushColorPost(POST_NONE);
			
			const bool filter = (mode & 0x4) == 0;
			
			gxSetTexture(image.surface->getTexture());
			{
				const int sx = image.surface->getWidth();
				const int sy = image.surface->getHeight();
				
				//logDebug("blit %d (%dx%d) -> %d. scale=%.2f, angle=%.2f", img, sx, sy, int(*m_gfx_dest), scale, rotate);
				
				const int dstX = (int)*m_gfx_x;
				const int dstY = (int)*m_gfx_y;
				
				gxSetTextureSampler(filter ? GX_SAMPLE_LINEAR : GX_SAMPLE_NEAREST, true);
				
				if (scale != 1.f || rotate != 0.f)
				{
					gxPushMatrix();
					gxTranslatef(dstX, dstY, 0);
					gxTranslatef(+sx/2.f, +sy/2.f, 0);
					gxScalef(scale, scale, 1.f);
					gxRotatef(rotate * 180.f / M_PI, 0, 0, 1);
					drawRect(-sx/2.f, -sy/2.f, +sx/2.f, +sy/2.f);
					gxPopMatrix();
				}
				else
				{
					drawRect(dstX, dstY, dstX + sx, dstY + sy);
				}
				
				gxSetTextureSampler(GX_SAMPLE_NEAREST, false);
			}
			gxSetTexture(0);
			popColorPost();
			popBlend();
		}
	}
}

void JsusFxGfx_Framework::gfx_blitext(EEL_F img, EEL_F * coords, EEL_F angle)
{
	STUB;
}

void JsusFxGfx_Framework::gfx_blitext2(int np, EEL_F ** parms, int blitmode)
{
	// blitmode:
	//     0 = blit
	//     1 = deltablit
	
	const int bmIndex = *parms[0];
	
	if (bmIndex < 0 || bmIndex >= imageCache.kMaxImages)
		return;
	
	const JsusFx_Image & image = imageCache.get(bmIndex);
	
	if (!image.isValid)
		return;
	
	IMAGE_SCOPE;
	
	PRIM_SCOPE(kPrimType_Other);
	
	const int destIndex = *m_gfx_dest;
	
	const int bmw = image.surface->getWidth();
	const int bmh = image.surface->getHeight();
	
	Assert(bmw >= 1 && bmh >= 1);
	
	// 0=img, 1=scale, 2=rotate
	
	double coords[8];
	
	const double scale = (blitmode == 0) ? parms[1][0] : 1.0;
	const double angle = (blitmode == 0) ? parms[2][0] : 0.0;
	
	if (blitmode == 0)
	{
		parms += 2;
		np -= 2;
	}

	coords[0] = np > 1 ? parms[1][0] : 0.f;
	coords[1] = np > 2 ? parms[2][0] : 0.f;
	coords[2] = np > 3 ? parms[3][0] : bmw;
	coords[3] = np > 4 ? parms[4][0] : bmh;
	coords[4] = np > 5 ? parms[5][0] : *m_gfx_x;
	coords[5] = np > 6 ? parms[6][0] : *m_gfx_y;
	coords[6] = np > 7 ? parms[7][0] : coords[2] * scale;
	coords[7] = np > 8 ? parms[8][0] : coords[3] * scale;

	const bool isFromFB = bmIndex == -1; // todo : allow blit from image index -1 ?
	(void)isFromFB;

	const bool isOverlapping = bmIndex == destIndex;
	
	Assert(!isOverlapping); // not handled for now
	
	if (isOverlapping)
		return;
	
	// todo : add overlap support

	if (blitmode == 1)
	{
		Assert(false);
		
	#if 0
		// todo : delta blit
		if (LICE_FUNCTION_VALID(LICE_DeltaBlit))
		{
			LICE_DeltaBlit(
				dest,bm,
				(int)coords[4],
				(int)coords[5],
				(int)coords[6],
				(int)coords[7],
				(float)coords[0],
				(float)coords[1],
				(float)coords[2],
				(float)coords[3],
				np > 9 ? (float)parms[9][0]:1.0f, // dsdx
				np > 10 ? (float)parms[10][0]:0.0f, // dtdx
				np > 11 ? (float)parms[11][0]:0.0f, // dsdy
				np > 12 ? (float)parms[12][0]:1.0f, // dtdy
				np > 13 ? (float)parms[13][0]:0.0f, // dsdxdy
				np > 14 ? (float)parms[14][0]:0.0f, // dtdxdy
				true,
				(float)*m_gfx_a,
				getCurModeForBlit(isFromFB));
		}
	#endif
	}
	else
	{
		const int mode = *m_gfx_mode;
	
		setColorf(1.f, 1.f, 1.f, *m_gfx_a); // pretty sure this is correct
	
		if (mode & 0x1)
			pushBlend(BLEND_ADD);
		else
			pushBlend(BLEND_ALPHA);
	
		if (mode & 0x2)
			pushColorPost(POST_SET_ALPHA_TO_ONE);
		else
			pushColorPost(POST_NONE);
	
	// todo : apply the correct filtering mode. currently there's an issue where LICE filtering and OpenGL filtering differ, causing artefacts around knobs for instance when they're being rotated. I suspect LICE clamps the sample position to the source rect
		//const bool filter = (mode & 0x4) == 0;
		const bool filter = false;
		
		const int dx = (int)coords[4];
		const int dy = (int)coords[5];
		const int dsx = (int)coords[6];
		const int dsy = (int)coords[7];
		
		// todo : not sure how to interpret rotation offset. it's not documented
		
		//const float rotOffsetX = np > 9 ? (float)parms[9][0] : 0.0f;
		//const float rotOffsetY = np > 10 ? (float)parms[10][0] : 0.0f;
		
		gxPushMatrix();
		{
			gxTranslatef(dx, dy, 0);
			gxTranslatef(+dsx/2.f, +dsy/2.f, 0.f);
			gxRotatef(angle * 180.f / M_PI, 0, 0, 1);
			gxTranslatef(-dsx/2.f, -dsy/2.f, 0.f);
			
			gxSetTexture(image.surface->getTexture());
			{
				gxSetTextureSampler(filter ? GX_SAMPLE_LINEAR : GX_SAMPLE_NEAREST, true);
				
				const float x1 = 0;
				const float y1 = 0;
				const float x2 = dsx;
				const float y2 = dsy;
				
				const float u1 = coords[0] / bmw;
				const float v1 = coords[1] / bmh;
				const float u2 = (coords[0] + coords[2]) / bmw;
				const float v2 = (coords[1] + coords[3]) / bmh;
				
			#if 0
				logDebug("blit2 %d (%d,%d %dx%d) -> %d (%d,%d %dx%d)",
					bmIndex, (int)coords[0], (int)coords[1], (int)coords[2], (int)coords[3],
					int(*m_gfx_dest), dx, dy, dsx, dsy);
			#endif
			
				gxBegin(GX_QUADS);
				{
					gxTexCoord2f(u1, v1); gxVertex2f(x1, y1);
					gxTexCoord2f(u2, v1); gxVertex2f(x2, y1);
					gxTexCoord2f(u2, v2); gxVertex2f(x2, y2);
					gxTexCoord2f(u1, v2); gxVertex2f(x1, y2);
				}
				gxEnd();
				
				gxSetTextureSampler(GX_SAMPLE_LINEAR, false);
			}
			gxSetTexture(0);
		}
		gxPopMatrix();
		
		popColorPost();
		popBlend();
	}
}

int JsusFxGfx_Framework::gfx_getchar(int p)
{
	if (p >= 2)
	{
		bool found = false;
		
		if (lastKey == p)
			found = true;
		
		return found ? 1 : 0;
	}
	else
	{
		const int result = lastKey;
		
		lastKey = 0;
		
		return result;
	}
}

#undef STUB
