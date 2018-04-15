#include "framework.h"

#include "jsusfx.h"
#include "jsusfx_gfx.h"

#include "StringEx.h"

#include <fstream>

const int GFX_SX = 640;
const int GFX_SY = 800;

struct JsusFxPathLibraryTest : JsusFxPathLibrary {
	static bool fileExists(const std::string &filename) {
		std::ifstream is(filename);
		return is.is_open();
	}

	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) override {
		const size_t pos = parentPath.rfind('/', '\\');
		if (pos != std::string::npos)
			resolvedPath = parentPath.substr(0, pos + 1);
		resolvedPath += importPath;
		return fileExists(resolvedPath);
	}
	
	virtual std::istream* open(const std::string &path) override {
		std::ifstream *stream = new std::ifstream(path);
		if ( stream->is_open() == false ) {
			delete stream;
			stream = nullptr;
		}
		
		return stream;
	}
	
	virtual void close(std::istream *&stream) override {
		delete stream;
		stream = nullptr;
	}
};

class JsusFxTest : public JsusFx {
public:
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        logDebug(output);
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        logError(output);
    }
};

#define STUB printf("function %s not implemented", __FUNCTION__)

struct JsusFxGfx_Framework : JsusFxGfx
{
	float m_fontSize = 12.f;
	
	virtual void setup(const int w, const int h) override
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
			
			glClearColor(r / 255.f, g / 255.f, b / 255.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		
		//*m_gfx_dest = -1.0;
		*m_gfx_a = 1.0;
		
		// update mouse state
		
		*m_mouse_x = mouse.x;
		*m_mouse_y = mouse.y;
		
		int vflags = 0;

		if (mouse.isDown(BUTTON_LEFT))
			vflags |= 0x1;
		if (mouse.isDown(BUTTON_RIGHT))
			vflags |= 0x2;

		*m_mouse_cap = vflags;
	}
	
	void updateColor()
	{
		setColorf(*m_gfx_r, *m_gfx_g, *m_gfx_b, *m_gfx_a);
	}
	
	virtual void gfx_line(int np, EEL_F ** params) override
	{
		const int x1 = (int)floor(params[0][0]);
		const int y1 = (int)floor(params[1][0]);
		const int x2 = (int)floor(params[2][0]);
		const int y2 = (int)floor(params[3][0]);
		
		drawLine(x1, y1, x2, y2);
	}
	
	virtual void gfx_rect(int np, EEL_F ** params) override
	{
		const int x1 = (int)floor(params[0][0]);
		const int y1 = (int)floor(params[1][0]);
		const int w = (int)floor(params[2][0]);
		const int h = (int)floor(params[3][0]);
		const int x2 = x1 + w;
		const int y2 = y1 + h;
		
  		const bool filled = (np < 5 || params[4][0] > 0.5);
		
  		if (filled)
  		{
  			updateColor();
			drawRect(x1, y1, x2, y2);
		}
		else
		{
			updateColor();
			drawRect(x1, y1, x2, y2);
		}
	}
	
	virtual void gfx_circle(EEL_F x, EEL_F y, EEL_F radius, bool fill, bool aa) override
	{
		if (fill)
		{
			updateColor();
			hqBegin(HQ_FILLED_CIRCLES);
			hqFillCircle(x, y, radius);
			hqEnd();
		}
		else
		{
			updateColor();
			hqBegin(HQ_STROKED_CIRCLES);
			hqStrokeCircle(x, y, radius, 1.f);
			hqEnd();
		}
	}
	
	virtual void gfx_triangle(EEL_F ** params, int np)
	{
		STUB;
	}
	
	virtual void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F useaa) override
	{
		updateColor();
		//drawLine(*m_gfx_x, *m_gfx_y, xpos, ypos);
		hqBegin(HQ_LINES);
		hqLine(*m_gfx_x, *m_gfx_y, 1.f, xpos, ypos, 1.f);
		hqEnd();
		
		*m_gfx_x = xpos;
		*m_gfx_y = ypos;
	}
	
	virtual void gfx_rectto(EEL_F xpos, EEL_F ypos) override
	{
		updateColor();
		drawRect(*m_gfx_x, *m_gfx_y, xpos, ypos);
		
		*m_gfx_x = xpos;
		*m_gfx_y = ypos;
	}
	
	virtual void gfx_arc(int np, EEL_F ** params)
	{
		STUB;
	}
	
	virtual void gfx_set(int np, EEL_F ** params) override
	{
		if (np < 1) return;

		if (m_gfx_r) *m_gfx_r = params[0][0];
		if (m_gfx_g) *m_gfx_g = np > 1 ? params[1][0] : params[0][0];
		if (m_gfx_b) *m_gfx_b = np > 2 ? params[2][0] : params[0][0];
		if (m_gfx_a) *m_gfx_a = np > 3 ? params[3][0] : 1.0;
		//if (m_gfx_mode) *m_gfx_mode = np > 4 ? params[4][0] : 0; // todo
		//if (np > 5 && m_gfx_dest) *m_gfx_dest = params[5][0]; // todo
	}
	
	virtual void gfx_roundrect(int np, EEL_F ** params)
	{
		STUB;
	}
	
	virtual void gfx_grad_or_muladd_rect(int mod, int np, EEL_F ** params) { }
	
	virtual void gfx_drawnumber(EEL_F n, int nd) override
	{
		char formatString[32];
		sprintf_s(formatString, sizeof(formatString), "%%.%df", nd);
		
		float sx;
		float sy;
		measureText(m_fontSize, sx, sy, formatString, n);
		
		updateColor();
		drawText(*m_gfx_x, *m_gfx_y + m_fontSize, m_fontSize, +1, -1, formatString, n);
		
		*m_gfx_x += sx;
	}
	
	virtual void gfx_drawchar(EEL_F n) override
	{
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
	
	virtual void gfx_drawstr(void * opaque, EEL_F ** parms, int np, int mode) // mode=1 for format, 2 for purely measure no format, 3 for measure char
	{
		STUB;
	}
	
	virtual void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b) override
	{
		setColorf(r, g, b, *m_gfx_a);
		drawPoint(*m_gfx_x, *m_gfx_y);
	}
	
	virtual void gfx_getpixel(EEL_F * r, EEL_F * g, EEL_F * b)
	{
		STUB;
	}

	virtual EEL_F gfx_loadimg(void * opaque, int img, EEL_F loadFrom) { return 0.f; }
	virtual void gfx_getimgdim(EEL_F img, EEL_F * w, EEL_F * h) { }
	virtual EEL_F gfx_setimgdim(int img, EEL_F * w, EEL_F * h) { return 0.f; }
	
	virtual EEL_F gfx_setfont(void * opaque, int np, EEL_F ** parms) { return 0.f; }
	virtual EEL_F gfx_getfont(void * opaque, int np, EEL_F ** parms) { return 0.f; }
	
	virtual EEL_F gfx_showmenu(void * opaque, EEL_F ** parms, int nparms)
	{
		STUB;
		
		return 0.f;
	}
	
	virtual EEL_F gfx_setcursor(void * opaque, EEL_F ** parms, int nparms)
	{
		STUB;
		
		return 0.f;
	}
	
	virtual void gfx_blurto(EEL_F x, EEL_F y)
	{
		STUB;
	}

	virtual void gfx_blit(EEL_F img, EEL_F scale, EEL_F rotate)
	{
		STUB;
	}
	
	virtual void gfx_blitext(EEL_F img, EEL_F * coords, EEL_F angle)
	{
		STUB;
	}
	
	virtual void gfx_blitext2(int mp, EEL_F ** params, int mode) // 0=blit, 1=deltablit
	{
		STUB;
	}
};

#undef STUB

static void doSlider(JsusFx & fx, Slider & slider, int x, int y)
{
	const int sx = 200;
	const int sy = 12;
	
	const bool isInside =
		//x >= 0 && x <= sx; // &&
		y >= 0 && y <= sy;
	
	if (isInside && mouse.isDown(BUTTON_LEFT))
	{
		const float t = x / float(sx);
		const float v = slider.min + (slider.max - slider.min) * t;
		fx.moveSlider(&slider - fx.sliders, v);
	}
	
	setColor(0, 0, 255, 127);
	const float t = (slider.getValue() - slider.min) / (slider.max - slider.min);
	drawRect(0, 0, sx * t, sy);
	
	setColor(colorWhite);
	drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.desc);
	
	setColor(colorWhite);
	drawRectLine(0, 0, sx, sy);
}

int main(int argc, char * argv[])
{
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	JsusFxTest fx;
	JsusFxGfx_Framework gfx;
	//JsusFxGfx_Log gfx;
	fx.gfx = &gfx;
	gfx.init(fx.m_vm);
	
	//const char * filename = "3bandpeakfilter";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/FocusPressPushZoom";
	//const char * filename = "/Users/thecat/jsusfx/scripts/liteon/vumetergfx";
	const char * filename = "/Users/thecat/jsusfx/scripts/liteon/statevariable";
	
	JsusFxPathLibraryTest pathLibrary;
	if (!fx.compile(pathLibrary, filename))
	{
		logError("failed to load file: %s", filename);
		return -1;
	}
	
	fx.prepare(44100, 64);
	
	double t = 0.0;
	double ts = 0.0;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			for (int a = 0; a < 10; ++a)
			{
				float ind[2][64];
				float * in[2] = { ind[0], ind[1] };
				float outd[2][64];
				float * out[2] = { outd[0], outd[1] };
				
				for (int i = 0; i < 64; ++i)
				{
					in[0][i] = cos(t) * 0.2;
					in[1][i] = sin(t) * 0.3 + random(-0.1, +0.1);
					
					double freq = lerp<double>(100.0, 1000.0, (sin(ts) + 1.0) / 2.0);
					t += 2.0 * M_PI * freq / 44100.0;
					ts += 2.0 * M_PI * 0.2 / 44100.0;
				}
				
				fx.process(in, out, 64);
			}
			
			gfx.setup(fx.gfx_w, fx.gfx_h);
			
			pushFontMode(FONT_SDF);
			fx.draw();
			popFontMode();
			
			const int sx = *gfx.m_gfx_w;
			const int sy = *gfx.m_gfx_h;
			
			int x = 10;
			int y = sy + 100;
			
			for (int i = 0; i < 64; ++i)
			{
				if (fx.sliders[i].exists)
				{
					gxPushMatrix();
					gxTranslatef(x, y, 0);
					doSlider(fx, fx.sliders[i], mouse.x - x, mouse.y - y);
					gxPopMatrix();
					
					y += 16;
				}
			}
		}
		framework.endDraw();
	}

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
