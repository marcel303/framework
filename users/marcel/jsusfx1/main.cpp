#include "framework.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include "Path.h"
#include "StringEx.h"

#include <fstream>

#include "WDL/wdlstring.h"

/*

GFX status:
- Most of Liteon scripts appear to work.
- JSFX-Kawa (https://github.com/kawaCat/JSFX-kawa) mostly works. Some text and blitting not working yet.
- ATK for Reaper loads ok, but doesn't output anything to the screen at all.

*/

/*

import filename -- REAPER v4.25+

You can specify a filename to import (this filename will be searched within the JS effect directory). Importing files via this directive will have any functions defined in their @init sections available to the local effect. Additionally, if the imported file implements other sections (such as @sample, etc), and the importing file does not implement those sections, the imported version of those sections will be used.

Note that files that are designed for import only (such as function libraries) should ideally be named xyz.jsfx-inc, as these will be ignored in the user FX list in REAPER.

---

mouse_cap

mouse_cap is a bitfield of mouse and keyboard modifier state.
	1: left mouse button
	2: right mouse button
	4: Control key (Windows), Command key (OSX)
	8: Shift key
	16: Alt key (Windows), Option key (OSX)
	32: Windows key (Windows), Control key (OSX) -- REAPER 4.60+
	64: middle mouse button -- REAPER 4.60+

mouse_wheel

gfx_texth
	avergage height of a text line

gfx_mode
	0x1 = additive
	0x2 = disable source alpha for gfx_blit (make it opaque)
	0x4 = disable filtering for gfx_blit (point sampling)

*/

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 64

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/"

const int GFX_SX = 1000;
const int GFX_SY = 720;

struct JsusFxPathLibraryTest : JsusFxPathLibrary {
	std::string dataRoot;
	
	JsusFxPathLibraryTest(const char * _dataRoot) {
		dataRoot = _dataRoot;
	}
	
	static bool fileExists(const std::string &filename) {
		std::ifstream is(filename);
		return is.is_open();
	}

	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) override {
		const size_t pos = parentPath.rfind('/', '\\');
		if (pos != std::string::npos)
			resolvedPath = parentPath.substr(0, pos + 1);
		if (fileExists(resolvedPath + importPath))
		{
			resolvedPath = resolvedPath + importPath;
			return true;
		}
		resolvedPath = resolvedPath + "lib/" + importPath; // fixme : this is a hack to make JSFX-Kawa work, without having to deal with search paths more generally
		if (fileExists(resolvedPath))
			return true;
		return false;
	}
	
	virtual bool resolveDataPath(const std::string &importPath, std::string &resolvedPath) override {
		resolvedPath = dataRoot + "/" + importPath;
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

static JsusFxTest * s_fx = nullptr;
static JsusFxPathLibrary * s_pathLibrary = nullptr;

#define STUB logDebug("function %s not implemented", __FUNCTION__)
//#define STUB do { } while (false)

struct JsusFx_Image
{
	bool isValid = false;
	
	Surface * surface = nullptr;
	
	~JsusFx_Image()
	{
		free();
	}
	
	void free()
	{
		if (surface != nullptr)
		{
			delete surface;
			surface = nullptr;
		}
		
		isValid = false;
	}
	
	void resize(const int sx, const int sy)
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
};

struct JsusFx_ImageCache
{
	static const int kMaxImages = 1024;
	
	JsusFx_Image images[kMaxImages];
	
	JsusFx_Image dummyImage;
	
	JsusFx_ImageCache()
	{
		dummyImage.resize(1, 1);
	}
	
	void free()
	{
		for (int i = 0; i < kMaxImages; ++i)
		{
			images[i].free();
		}
	}
	
	int alloc(const int sx, const int sy)
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
	
	JsusFx_Image & get(const int index)
	{
		Assert(index != -1);
		
		if (index < 0 || index >= kMaxImages)
			return dummyImage;
		else
			return images[index];
	}
};

#define PUSH_FRAMEBUFFER 1

struct JsusFx_ImageScope
{
	JsusFx_Image * image = nullptr;
	
	JsusFx_ImageScope(JsusFx_ImageCache & cache, int index)
	{
	#if PUSH_FRAMEBUFFER
		if (index == -1)
			pushSurface(nullptr);
		else
	#else
		if (index != -1)
	#endif
		{
			image = &cache.get(index);
			Assert(image->isValid);
			pushSurface(image->surface);
		}
	}
	
	~JsusFx_ImageScope()
	{
	#if PUSH_FRAMEBUFFER
		popSurface();
	#else
		if (image != nullptr)
			popSurface();
	#endif
	}
};

#define IMAGE_SCOPE JsusFx_ImageScope imageScope(imageCache, *m_gfx_dest)
//#define IMAGE_SCOPE do { } while (false)

struct JsusFx_BlendScope
{
	JsusFx_BlendScope(const int mode)
	{
		if (mode & 0x1)
			pushBlend(BLEND_ADD);
		else
			pushBlend(BLEND_ALPHA);
	}
	
	~JsusFx_BlendScope()
	{
		popBlend();
	}
};

#define BLEND_SCOPE JsusFx_BlendScope blendScope(*m_gfx_mode)

#define LINE_STROKE 1.f

#include "audio.h"
#include "FileStream.h"
#include "StreamReader.h"
#include "StringEx.h"

struct JsusFx_FileInfo
{
	std::string filename;
	
	bool isValid() const
	{
		return !filename.empty();
	}
	
	bool init(const char * _filename)
	{
		filename = _filename;
		
		return isValid();
	}
};

struct JsusFx_File
{
	enum Mode
	{
		kMode_None,
		kMode_Text,
		kMode_Sound
	};
	
	std::string filename;
	Mode mode = kMode_None;
	
	SoundData * soundData = nullptr;
	
	int readPosition = 0;
	
	std::vector<EEL_F> vars;
	
	~JsusFx_File()
	{
		close();
	}
	
	bool open(const char * _filename)
	{
		// reset
		
		filename.clear();
		
		// check if file exists
		
		FILE * file = fopen(_filename, "rb");
		
		if (file == nullptr)
			return false;
		else
		{
			filename = _filename;
			fclose(file);
			return true;
		}
	}
	
	void close()
	{
		vars.clear();
		
		delete soundData;
		soundData = nullptr;
	}
	
	bool riff(int & numChannels, int & sampleRate)
	{
		Assert(mode == kMode_None);
		
		// reset read state
		
		mode = kMode_None;
		readPosition = 0;
		
		// load RIFF file
		
		soundData = loadSound(filename.c_str());
		
		if (soundData == nullptr || (soundData->channelSize != 2 && soundData->channelSize != 4))
		{
			numChannels = 0;
			sampleRate = 0;
			return false;
		}
		else
		{
			mode = kMode_Sound;
			numChannels = soundData->channelCount;
			sampleRate = soundData->sampleRate;
			return true;
		}
	}
	
	bool text()
	{
		Assert(mode == kMode_None);
		
		// reset read state
		
		mode = kMode_None;
		readPosition = 0;
		
		// load text file
		
		std::istream * stream = nullptr;
		
		try
		{
			stream = s_pathLibrary->open(filename);
			
			if (stream == nullptr)
			{
				logError("failed to open text file");
				return false;
			}
			
			while (!stream->eof())
			{
				char line[2048];
				
				stream->getline(line, sizeof(line), '\n');
				
				// a poor way of skipping comments. assume / is the start of // and strip anything that come after it
				char * pos = strchr(line, '/');
				if (pos != nullptr)
					*pos = 0;
				
				const char * p = line;
				
				for (;;)
				{
					// skip trailing white space
					
					while (*p && isspace(*p))
						p++;
					
					// reached end of the line ?
					
					if (*p == 0)
						break;
					
					// parse the value
					
					double var;
					
					if (sscanf(p, "%lf", &var) == 1)
						vars.push_back(var);
					
					// skip the value
					
					while (*p && !isspace(*p))
						p++;
				}
			}
			
			s_pathLibrary->close(stream);
			
			mode = kMode_Text;
			
			return true;
		}
		catch (std::exception & e)
		{
			if (stream != nullptr)
			{
				delete stream;
				stream = nullptr;
			}
			
			logError("failed to read text file contents: %s", e.what());
			return false;
		}
	}
	
	int avail() const
	{
		if (mode == kMode_None)
			return 0;
		else if (mode == kMode_Text)
			return readPosition == vars.size() ? 0 : 1;
		else if (mode == kMode_Sound)
			return soundData->sampleCount * soundData->channelCount - readPosition;
		else
			return 0;
	}
	
	bool mem(const int numValues, EEL_F * dest)
	{
		if (mode == kMode_None)
			return false;
		else if (mode == kMode_Text)
			return false;
		else if (mode == kMode_Sound)
		{
			if (numValues > avail())
				return false;
			for (int i = 0; i < numValues; ++i)
			{
				const int channelIndex = readPosition / soundData->sampleCount;
				const int sampleIndex = readPosition % soundData->sampleCount;
				
				if (soundData->channelSize == 2)
				{
					const short * values = (short*)soundData->sampleData;
					
					dest[i] = values[sampleIndex * soundData->channelCount + channelIndex] / float(1 << 15);
				}
				else if (soundData->channelSize == 4)
				{
					const float * values = (float*)soundData->sampleData;
					
					dest[i] = values[sampleIndex * soundData->channelCount + channelIndex];
				}
				else
				{
					Assert(false);
				}
				
				readPosition++;
			}
			
			return true;
		}
		else
			return false;
	}
	
	bool var(EEL_F & value)
	{
		if (mode == kMode_None)
			return false;
		else if (mode == kMode_Text)
		{
			if (avail() < 1)
				return false;
			value = vars[readPosition];
			readPosition++;
			return true;
		}
		else if (mode == kMode_Sound)
			return false;
		else
			return false;
	}
};

static const int kMaxFileInfos = 128;
static const int kMaxFileHandles = 16;

static JsusFx_FileInfo s_fileInfos[kMaxFileInfos];

static JsusFx_File * s_fileHandles[kMaxFileHandles] = { };

struct JsusFxGfx_Framework : JsusFxGfx
{
	float m_fontSize = 10.f;
	
	JsusFx_ImageCache imageCache;
	
	int mouseFlags = 0;
	
	virtual bool handleFile(int index, const char *filename) override
	{
		if (index < 0 || index >= kMaxFileInfos)
		{
			logError("file index out of bounds %d:%s", index, filename);
			return false;
		}
		
		if (s_fileInfos[index].isValid())
		{
			logWarning("file already exists %d:%s", index, filename);
			
			s_fileInfos[index] = JsusFx_FileInfo();
		}
		
		//
		
		if (s_fileInfos[index].init(filename))
		{
			if (Path::GetExtension(filename, true) == "png" || Path::GetExtension(filename, true) == "jpg")
			{
				gfx_loadimg(nullptr, index, index);
			}
			
			return true;
		}
		else
		{
			logError("failed to find file %d:%s", index, filename);
			
			return false;
		}
	}
		
	virtual void setup(const int w, const int h) override
	{
		// update gfx state
		
		*m_gfx_w = w ? w : GFX_SX;
		*m_gfx_h = h ? h : GFX_SY/4;
		
		if (*m_gfx_clear > -1.0)
		{
			const int a = (int)*m_gfx_clear;
			
			const int r = (a >>  0) & 0xff;
			const int g = (a >>  8) & 0xff;
			const int b = (a >> 16) & 0xff;
			
			glClearColor(r / 255.f, g / 255.f, b / 255.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		
		*m_gfx_texth = m_fontSize;
		
		*m_gfx_dest = -1.0;
		*m_gfx_a = 1.0;
		
		// update mouse state
		
		*m_mouse_x = mouse.x;
		*m_mouse_y = mouse.y;
		
		const bool isInside =
			mouse.x >= 0 && mouse.x < *m_gfx_w &&
			mouse.y >= 0 && mouse.y < *m_gfx_h;
		
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
		
		int vflags = 0;

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

		*m_mouse_cap = vflags;
	}
	
	virtual void handleReset()
	{
		m_fontSize = 12.f;
		
		imageCache.free();
	}
	
	void updateColor()
	{
		setColorf(*m_gfx_r, *m_gfx_g, *m_gfx_b, *m_gfx_a);
	}
	
	virtual void gfx_line(int np, EEL_F ** params) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
		const int x1 = (int)floor(params[0][0]);
		const int y1 = (int)floor(params[1][0]);
		const int x2 = (int)floor(params[2][0]);
		const int y2 = (int)floor(params[3][0]);
		
		updateColor();
		hqBegin(HQ_LINES);
		hqLine(x1, y1, LINE_STROKE, x2, y2, LINE_STROKE);
		hqEnd();
	}
	
	virtual void gfx_rect(int np, EEL_F ** params) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
			drawRectLine(x1, y1, x2, y2);
		}
	}
	
	virtual void gfx_circle(EEL_F x, EEL_F y, EEL_F radius, bool fill, bool aa) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
	
	virtual void gfx_triangle(EEL_F ** parms, int np) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
	
	virtual void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F useaa) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
		updateColor();
		hqBegin(HQ_LINES);
		hqLine(*m_gfx_x, *m_gfx_y, LINE_STROKE, xpos, ypos, LINE_STROKE);
		hqEnd();
		
		*m_gfx_x = xpos;
		*m_gfx_y = ypos;
	}
	
	virtual void gfx_rectto(EEL_F xpos, EEL_F ypos) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
		updateColor();
		drawRect(*m_gfx_x, *m_gfx_y, xpos, ypos);
		
		*m_gfx_x = xpos;
		*m_gfx_y = ypos;
	}
	
	virtual void gfx_arc(int np, EEL_F ** parms)
	{
		STUB;
		
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
	
	virtual void gfx_set(int np, EEL_F ** params) override
	{
		if (np < 1) return;

		if (m_gfx_r) *m_gfx_r = params[0][0];
		if (m_gfx_g) *m_gfx_g = np > 1 ? params[1][0] : params[0][0];
		if (m_gfx_b) *m_gfx_b = np > 2 ? params[2][0] : params[0][0];
		if (m_gfx_a) *m_gfx_a = np > 3 ? params[3][0] : 1.0;
		if (m_gfx_mode) *m_gfx_mode = np > 4 ? params[4][0] : 0;
		if (np > 5 && m_gfx_dest) *m_gfx_dest = params[5][0];
	}
	
	virtual void gfx_roundrect(int np, EEL_F ** parms) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;

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
	
	virtual void gfx_grad_or_muladd_rect(int mode, int np, EEL_F ** parms) override
	{
		const int x1 = (int)floor(parms[0][0]);
		const int y1 = (int)floor(parms[1][0]);
		const int w = (int)floor(parms[2][0]);
		const int h = (int)floor(parms[3][0]);
		
		if (w <= 0 || h <= 0)
			return;
		
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
	
	virtual void gfx_drawnumber(EEL_F n, int nd) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
	
	virtual void gfx_drawstr(void * opaque, EEL_F ** parms, int nparms, int formatmode) override // mode=1 for format, 2 for purely measure no format, 3 for measure char
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
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
			s = s_fx->getString(index, &fs);
			
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
	
	virtual void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b) override
	{
		IMAGE_SCOPE;
		BLEND_SCOPE;
		
		setColorf(r, g, b, *m_gfx_a);
		drawPoint(*m_gfx_x, *m_gfx_y);
	}
	
	virtual void gfx_getpixel(EEL_F * r, EEL_F * g, EEL_F * b) override
	{
		IMAGE_SCOPE;
		
		const int x = *m_gfx_x;
		const int y = *m_gfx_y;
		
		uint8_t rgba[4];
		
		glReadPixels(
			x, y,
			1, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			rgba);
		checkErrorGL();
		
		*r = rgba[0] / 255.f;
		*g = rgba[1] / 255.f;
		*b = rgba[2] / 255.f;
	}

	virtual EEL_F gfx_loadimg(void * opaque, int index, EEL_F loadFrom) override
	{
		const int fileIndex = (int)loadFrom;
		
		if (fileIndex < 0 || fileIndex >= kMaxFileInfos)
			return -1;
	
		if (s_fileInfos[fileIndex].isValid() == false)
			return -1;
		
		if (index >= 0 && index < imageCache.kMaxImages)
		{
			JsusFx_Image & image = imageCache.images[index];
			
			const char * filename = s_fileInfos[fileIndex].filename.c_str();
			
			const GLuint texture = getTexture(filename);
			
			if (texture == 0)
			{
				logError("failed to load image %d:%s", index, filename);
				
				return -1;
			}
			else
			{
				GLuint restoreTexture;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
				checkErrorGL();
			
				GLint sx;
				GLint sy;
				
				glBindTexture(GL_TEXTURE_2D, texture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sx);
				checkErrorGL();
				
				glBindTexture(GL_TEXTURE_2D, texture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sy);
				checkErrorGL();
			
				glBindTexture(GL_TEXTURE_2D, restoreTexture);
				
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
	
	virtual void gfx_getimgdim(EEL_F _img, EEL_F * w, EEL_F * h) override
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
	
	virtual EEL_F gfx_setimgdim(int img, EEL_F * w, EEL_F * h) override
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
			
			logDebug("resizing image %d to (%d, %d)", img, use_w, use_h);
			
			image.resize(use_w, use_h);
		}
		
	 	return 1.f;
	}
	
	virtual EEL_F gfx_setfont(void * opaque, int np, EEL_F ** parms) override
	{
		const int size = np > 2 ? (int)parms[2][0] : 10;
		
		// note : this implementation is very basic and doesn't support selecting a different font face
		
		m_fontSize = size;
		
		return 0.f;
	}
	
	virtual EEL_F gfx_getfont(void * opaque, int np, EEL_F ** parms)
	{
		STUB;
		
		return 0.f;
	}
	
	virtual EEL_F gfx_showmenu(void * opaque, EEL_F ** parms, int nparms)
	{
		STUB;
		
		return 0.f;
	}
	
	virtual EEL_F gfx_setcursor(void * opaque, EEL_F ** parms, int nparms) override
	{
	#if 1
		const int cursorResId = (int)parms[0][0];
		
		char cursorName[64];
		cursorName[0]= 0;
		
		if (nparms > 1)
		{
			WDL_FastString * fs = nullptr;
			const char * p = s_fx->getString(parms[1][0], &fs);
			
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
	
	virtual void gfx_blurto(EEL_F x, EEL_F y) override
	{
		STUB;
		
		*m_gfx_x = x;
		*m_gfx_y = y;
	}

	virtual void gfx_blit(EEL_F _img, EEL_F scale, EEL_F rotate) override
	{
		int img = (int)_img;
		
		Assert(img != -1);
		
		if (img >= 0 && img < imageCache.kMaxImages)
		{
			const JsusFx_Image & image = imageCache.images[img];
			
			Assert(image.isValid);
			
			if (image.isValid)
			{
				IMAGE_SCOPE;
				
				const int mode = *m_gfx_mode;
				
				Assert(img != (int)*m_gfx_dest);
				
				setColorf(1.f, 1.f, 1.f, *m_gfx_a); // pretty sure this is correct
				
				// I think blend should be ALPHA or ADD depending on gfx_mode
				//pushBlend(BLEND_PREMULTIPLIED_ALPHA);
				pushBlend(BLEND_ALPHA);
				//pushBlend(BLEND_ADD);
				
				gxSetTexture(image.surface->getTexture());
				{
					const int sx = image.surface->getWidth();
					const int sy = image.surface->getHeight();
					
					logDebug("blit %d (%dx%d) -> %d. scale=%.2f, angle=%.2f", img, sx, sy, int(*m_gfx_dest), scale, rotate);
					
					const int dstX = (int)*m_gfx_x;
					const int dstY = (int)*m_gfx_y;
					
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				
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
				}
				gxSetTexture(0);
				popBlend();
			}
		}
	}
	
	virtual void gfx_blitext(EEL_F img, EEL_F * coords, EEL_F angle)
	{
		STUB;
	}
	
	virtual void gfx_blitext2(int np, EEL_F ** parms, int blitmode) override
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
			// todo : apply blend mode (gfx_mode)
			
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
				
				setColorf(1.f, 1.f, 1.f, *m_gfx_a);
				gxSetTexture(image.surface->getTexture());
				{
					// todo : apply the correct filtering mode
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					
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
				
					gxBegin(GL_QUADS);
					{
						gxTexCoord2f(u1, v1); gxVertex2f(x1, y1);
						gxTexCoord2f(u2, v1); gxVertex2f(x2, y1);
						gxTexCoord2f(u2, v2); gxVertex2f(x2, y2);
						gxTexCoord2f(u1, v2); gxVertex2f(x1, y2);
					}
					gxEnd();
				}
				gxSetTexture(0);
			}
			gxPopMatrix();
		}
	}
};

#undef STUB

static void doSlider(JsusFx & fx, JsusFx_Slider & slider, int x, int y)
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
	
	if (slider.isEnum)
	{
		const int enumIndex = (int)slider.getValue();
		
		if (enumIndex >= 0 && enumIndex < slider.enumNames.size())
		{
			setColor(colorWhite);
			drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.enumNames[enumIndex].c_str());
		}
	}
	else
	{
		setColor(colorWhite);
		drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.desc);
	}
	
	setColor(63, 31, 255, 127);
	drawRectLine(0, 0, sx, sy);
}

//

struct MyFileAPI : JsusFxFileAPI
{
	JsusFxPathLibrary & pathLibrary;
	
	MyFileAPI(JsusFxPathLibrary & _pathLibrary)
		: pathLibrary(_pathLibrary)
	{
	}
	
	virtual int file_open(const char * filename) override
	{
		std::string resolvedPath;
		
		if (pathLibrary.resolveDataPath(filename, resolvedPath) == false)
		{
			logError("failed to resolve data path");
			return -1;
		}
		
		for (int i = 1; i < kMaxFileHandles; ++i)
		{
			if (s_fileHandles[i] == nullptr)
			{
				s_fileHandles[i] = new JsusFx_File();
				
				if (s_fileHandles[i]->open(resolvedPath.c_str()) == false)
				{
					logError("failed to open file: %s", resolvedPath.c_str());
					
					delete s_fileHandles[i];
					s_fileHandles[i] = nullptr;
					
					return -1;
				}
				else
				{
					return i;
				}
			}
		}
		
		logError("failed to find a free file handle");
		return -1;
	}

	virtual bool file_close(const int index) override
	{
		if (index < 0 || index >= kMaxFileHandles)
		{
			logError("invalid file handle");
			return -1;
		}
		
		if (s_fileHandles[index] == nullptr)
		{
			logError("file not opened");
			return -1;
		}
		
		s_fileHandles[index]->close();
		
		delete s_fileHandles[index];
		s_fileHandles[index] = nullptr;
		
		return 0;
	}

	virtual int file_avail(const int index) override
	{
		if (index < 0 || index >= kMaxFileHandles)
		{
			logError("invalid file handle");
			return 0;
		}
		
		if (s_fileHandles[index] == nullptr)
		{
			logError("file not opened");
			return 0;
		}
		
		return s_fileHandles[index]->avail();
	}

	virtual bool file_riff(const int index, int & numChannels, int & sampleRate) override
	{
		if (index < 0 || index >= kMaxFileHandles)
		{
			logError("invalid file handle");
			return false;
		}
		
		if (s_fileHandles[index] == nullptr)
		{
			logError("file not opened");
			return false;
		}
		
		if (s_fileHandles[index]->riff(numChannels, sampleRate) == false)
		{
			logError("failed to parse RIFF");
			return false;
		}
		
		return true;
	}

	virtual bool file_text(const int index) override
	{
		if (index < 0 || index >= kMaxFileHandles)
		{
			logError("invalid file handle");
			return false;
		}
		
		if (s_fileHandles[index] == nullptr)
		{
			logError("file not opened");
			return false;
		}
		
		if (s_fileHandles[index]->text() == false)
		{
			logError("failed to parse text");
			return false;
		}
		
		return true;
	}

	virtual int file_mem(const int index, EEL_F * dest, const int numValues) override
	{
		if (index < 0 || index >= kMaxFileHandles)
		{
			logError("invalid file handle");
			return 0;
		}
		
		if (s_fileHandles[index] == nullptr)
		{
			logError("file not opened");
			return 0;
		}
		
		if (s_fileHandles[index]->mem(numValues, dest) == false)
		{
			logError("failed to read data");
			return 0;
		}
		else
			return numValues;
	}

	virtual bool file_var(const int index, EEL_F & dest) override
	{
		if (index < 0 || index >= kMaxFileHandles)
		{
			logError("invalid file handle");
			return 0;
		}
		
		if (s_fileHandles[index] == nullptr)
		{
			logError("file not opened");
			return 0;
		}
		
		if (s_fileHandles[index]->var(dest) == false)
		{
			logError("failed to read value");
			return 0;
		}
		else
			return 1;
	}
};

//

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStream.h"
#include "audiostream/AudioStreamVorbis.h"

struct AudioStream_JsusFx : AudioStream
{
	AudioStream * source = nullptr;
	
	JsusFx * fx = nullptr;
	
	SDL_mutex * mutex = nullptr;
	
	virtual ~AudioStream_JsusFx() override
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}
	
	void init(AudioStream * _source, JsusFx * _fx)
	{
		source = _source;
		
		fx = _fx;
		
		mutex = SDL_CreateMutex();
	}
	
	void lock()
	{
		SDL_LockMutex(mutex);
	}
	
	void unlock()
	{
		SDL_UnlockMutex(mutex);
	}
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		Assert(numSamples == BUFFER_SIZE);
		
		source->Provide(numSamples, buffer);
		
		float inputL[BUFFER_SIZE];
		float inputR[BUFFER_SIZE];
		float outputL[BUFFER_SIZE];
		float outputR[BUFFER_SIZE];
		
		for (int i = 0; i < numSamples; ++i)
		{
			inputL[i] = buffer[i].channel[0] / float(1 << 15);
			inputR[i] = buffer[i].channel[1] / float(1 << 15);
		}
		
		float * input[2] =
		{
			inputL,
			inputR
		};
		
		float * output[2] =
		{
			outputL,
			outputR
		};
		
		lock();
		{
			fx->process(input, output, numSamples, 2, 2);
		}
		unlock();
		
		for (int i = 0; i < numSamples; ++i)
		{
			buffer[i].channel[0] = outputL[i] * float(1 << 15);
			buffer[i].channel[1] = outputR[i] * float(1 << 15);
		}
		
		return numSamples;
	}
};

//

static AudioStream_JsusFx * s_audioStream = nullptr;

static void handleAction(const std::string & action, const Dictionary & d)
{
	if (action == "filedrop")
	{
		s_audioStream->lock();
		{
			auto filename = d.getString("file", "");
			
			JsusFxPathLibraryTest pathLibrary(DATA_ROOT);
			s_fx->compile(pathLibrary, filename);
			
			s_fx->prepare(SAMPLE_RATE, BUFFER_SIZE);
		}
		s_audioStream->unlock();
	}
}

int main(int argc, char * argv[])
{
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	JsusFx::init();
	
	JsusFxPathLibraryTest pathLibrary(DATA_ROOT);
	s_pathLibrary = &pathLibrary;
	
	JsusFxTest fx;
	s_fx = &fx;
	JsusFxGfx_Framework gfx;
	//JsusFxGfx_Log gfx;
	fx.gfx = &gfx;
	gfx.init(fx.m_vm);
	
	MyFileAPI fileAPI(pathLibrary);
	fx.fileAPI = &fileAPI;
	fileAPI.init(fx.m_vm);
	
	//const char * filename = "3bandpeakfilter";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/FocusPressPushZoom";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/Direct";
	//const char * filename = "/Users/thecat/jsusfx/scripts/liteon/vumetergfx";
	//const char * filename = "/Users/thecat/jsusfx/scripts/liteon/statevariable";
	//const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Delay.jsfx";
	//const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Chorus.jsfx";
	//const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Flanger.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Spring-Box.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Stereo Alignment Delay.jsfx";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/RotateTiltTumble";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Bad Connection.jsfx";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Encode/Quad";
	const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Encode/AmbiXToB";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Decode/Binaural";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Encode/Periphonic3D";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Decode/UHJ";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Warble.jsfx"; // fixme
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Spectrum Matcher.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Smooth Limiter.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Sandwich Amp.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Pulsar.jsfx"; // todo
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Panalysis.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/pad-synth.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/MIDI Harmony.jsfx"; // todo
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Level Meter.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Learning Sampler.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Humonica.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Hammer And String.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Hammer And Chord.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Echo-Cycles.jsfx";
	
	if (!fx.compile(pathLibrary, filename))
	{
		logError("failed to load file: %s", filename);
		return -1;
	}
	
	fx.prepare(SAMPLE_RATE, BUFFER_SIZE);
	
	AudioStream_Vorbis vorbis;
	vorbis.Open("movers.ogg", true);
	
	AudioStream_JsusFx audioStream;
	audioStream.init(&vorbis, &fx);
	s_audioStream = &audioStream;
	
	AudioOutput_PortAudio audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, BUFFER_SIZE);
	audioOutput.Play(&audioStream);
	
	double t = 0.0;
	double ts = 0.0;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (keyboard.wentDown(SDLK_SPACE))
		{
			if (!fx.compile(pathLibrary, filename))
			{
				logError("failed to load file: %s", filename);
				return -1;
			}
			
			fx.prepare(SAMPLE_RATE, BUFFER_SIZE);
		}
		
		if (framework.quitRequested)
			break;

		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
		#if 0
			for (int a = 0; a < 10; ++a)
			{
				float ind[2][BUFFER_SIZE];
				float * in[2] = { ind[0], ind[1] };
				float outd[2][BUFFER_SIZE];
				float * out[2] = { outd[0], outd[1] };
				
				for (int i = 0; i < BUFFER_SIZE; ++i)
				{
					in[0][i] = cos(t) * 0.2;
					in[1][i] = sin(t) * 0.3 + random(-0.1, +0.1);
					
					double freq = lerp<double>(100.0, 1000.0, (sin(ts) + 1.0) / 2.0);
					t += 2.0 * M_PI * freq / double(SAMPLE_RATE);
					ts += 2.0 * M_PI * 0.2 / double(SAMPLE_RATE);
				}
				
				fx.process(in, out, 64);
			}
		#endif
			
			gfx.setup(fx.gfx_w, fx.gfx_h);
			
			//setDrawRect(0, 0, fx.gfx_w, fx.gfx_h);
			pushFontMode(FONT_SDF);
			setColorClamp(true);
			fx.draw();
			setColorClamp(false);
			popFontMode();
			//clearDrawRect();
			
		#if 0
			{
				int x = 0;
				int index = 0;
				
				for (auto & image : gfx.imageCache.images)
				{
					if (image.isValid)
					{
						setColor(colorWhite);
						gxSetTexture(image.surface->getTexture());
						drawRect(x, 0, x + 40, 40);
						gxSetTexture(0);
						
						drawText(x + 4, 4, 10, +1, +1, "%d", index);
						
						setColor(100, 100, 10);
						drawRectLine(x, 0, x + 40, 40);
						
						x += 40;
					}
					
					index++;
				}
			}
		#endif
			
			const int sx = *gfx.m_gfx_w;
			const int sy = *gfx.m_gfx_h;
			
			int x = 10;
			int y = sy + 100;
			
			setColor(160, 160, 160);
			drawText(x + 240, y, 18, +1, +1, "JSFX file: %s", Path::GetFileName(filename).c_str());
			drawText(x + 240 + 3, y + 21, 12, +1, +1, "%d ins, %d outs", fx.numInputs, fx.numOutputs);
			
			for (int i = 0; i < 64; ++i)
			{
				if (fx.sliders[i].exists && fx.sliders[i].desc[0] != '-')
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
