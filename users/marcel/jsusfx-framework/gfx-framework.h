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

#include "jsusfx_gfx.h"

//

class Surface;

struct JsusFx_Image;
struct JsusFx_ImageCache;
struct JsusFxGfx_Framework; // implements JsusFxGfx

//

struct JsusFx_Image
{
	bool isValid = false;
	
	Surface * surface = nullptr;
	
	~JsusFx_Image();
	
	void free();
	void resize(const int sx, const int sy);
};

struct JsusFx_ImageCache
{
	static const int kMaxImages = 1024;
	
	JsusFx_Image images[kMaxImages];
	
	JsusFx_Image dummyImage;
	
	JsusFx_ImageCache();
	
	void free();
	int alloc(const int sx, const int sy);
	JsusFx_Image & get(const int index);
};

//

struct JsusFxGfx_Framework : JsusFxGfx
{
	enum PrimType
	{
		kPrimType_Other,
		kPrimType_Rect,
		kPrimType_RectLine,
		kPrimType_HqLine,
		kPrimType_HqFillCircle,
		kPrimType_HqStrokeCircle
	};
	
	float m_fontSize = 10.f;
	
	JsusFx_ImageCache imageCache;
	
	int mouseFlags = 0;
	
	int currentImageIndex = -1;
	int currentBlendMode = -1;
	
	JsusFx & jsusFx;
	
	PrimType primType = kPrimType_Other;
	
	JsusFxGfx_Framework(JsusFx & _jsusFx);
	
	virtual void setup(const int w, const int h) override;
	
	virtual void beginDraw() override;
	virtual void endDraw() override;
	
	virtual void handleReset(); // todo : mark override and add virtual to JsusFxGfx; called by JsusFx when code is released
	
	void updatePrimType(PrimType _primType);
	void updateBlendMode();
	void updateColor();
	void updateSurface();
	
	virtual void gfx_line(int np, EEL_F ** params) override;
	virtual void gfx_rect(int np, EEL_F ** params) override;
	virtual void gfx_circle(EEL_F x, EEL_F y, EEL_F radius, bool fill, bool aa) override;
	virtual void gfx_triangle(EEL_F ** parms, int np) override;
	virtual void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F useaa) override;
	virtual void gfx_rectto(EEL_F xpos, EEL_F ypos) override;
	virtual void gfx_arc(int np, EEL_F ** parms);
	virtual void gfx_set(int np, EEL_F ** params) override;
	virtual void gfx_roundrect(int np, EEL_F ** parms) override;
	virtual void gfx_grad_or_muladd_rect(int mode, int np, EEL_F ** parms) override;
	virtual void gfx_drawnumber(EEL_F n, int nd) override;
	virtual void gfx_drawchar(EEL_F n) override;
	virtual void gfx_drawstr(void * opaque, EEL_F ** parms, int nparms, int formatmode) override;
	virtual void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b) override;
	virtual void gfx_getpixel(EEL_F * r, EEL_F * g, EEL_F * b) override;
	virtual EEL_F gfx_loadimg(JsusFx & jsusFx, int index, EEL_F loadFrom) override;
	virtual void gfx_getimgdim(EEL_F _img, EEL_F * w, EEL_F * h) override;
	virtual EEL_F gfx_setimgdim(int img, EEL_F * w, EEL_F * h) override;
	virtual EEL_F gfx_setfont(int np, EEL_F ** parms) override;
	virtual EEL_F gfx_getfont(int np, EEL_F ** parms) override;
	virtual EEL_F gfx_showmenu(EEL_F ** parms, int nparms) override;
	virtual EEL_F gfx_setcursor(EEL_F ** parms, int nparms) override;
	virtual void gfx_blurto(EEL_F x, EEL_F y) override;
	virtual void gfx_blit(EEL_F _img, EEL_F scale, EEL_F rotate) override;
	virtual void gfx_blitext(EEL_F img, EEL_F * coords, EEL_F angle) override;
	virtual void gfx_blitext2(int np, EEL_F ** parms, int blitmode) override;
};
