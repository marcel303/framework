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

#pragma once

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#else
	#include <GL/glew.h>
#endif

#include <map>
#include <SDL2/SDL.h>
#include <string>
#include "framework.h"
#include "internal_filereader.h"

#if !defined(USE_FREETYPE)
	#if defined(IPHONEOS)
		#define USE_FREETYPE 1
	#else
		#define USE_FREETYPE 1 // do not alter
	#endif
#endif

#define USE_STBFONT 0

#if FRAMEWORK_USE_OPENAL
	#include <OpenAL/al.h>
#endif

#if !defined(USE_GLYPH_ATLAS)
	#if (ENABLE_OPENGL && !USE_LEGACY_OPENGL) || ENABLE_METAL
		#define USE_GLYPH_ATLAS 1
	#else
		#define USE_GLYPH_ATLAS 0 // cannot use glyph cache, as it uses R8 texture storage
	#endif
#endif

#if !defined(ENABLE_MSDF_FONTS)
	#if (ENABLE_OPENGL && !USE_LEGACY_OPENGL) || ENABLE_METAL
		#define ENABLE_MSDF_FONTS 1
	#else
		#define ENABLE_MSDF_FONTS 0 // cannot use MSDF fonts as the shader is too complex for the legacy OpenGL mode
	#endif
#endif

#if USE_GLYPH_ATLAS
	#define GLYPH_ATLAS_BORDER 1
#endif

#if ENABLE_MSDF_FONTS
	#define MSDF_GLYPH_PADDING_INNER 3
	#define MSDF_GLYPH_PADDING_OUTER 4
	#define MSDF_SCALE .04f
#endif

#if USE_FREETYPE
	#include <ft2build.h>
	#include FT_FREETYPE_H
#endif

#if USE_STBFONT || ENABLE_MSDF_FONTS
	#include "stb_truetype.h"
#endif

#ifdef WIN32
	#define ENABLE_EVENTS_WORKAROUND 1
#else
	#define ENABLE_EVENTS_WORKAROUND 0
#endif

#if ENABLE_OPENGL
	#include "gx-opengl/shaderCache.h"
#endif
#if ENABLE_METAL
	#include "gx-metal/shaderCache.h"
#endif

#ifndef WIN32
#include <errno.h> // EINVAL
#include <stdio.h>
inline int fopen_s(FILE ** file, const char * filename, const char * mode)
{
	*file = fopen(filename, mode);
	return *file ? 0 : EINVAL;
}
#define sprintf_s(s, ss, f, ...) snprintf(s, ss, f, __VA_ARGS__)
#define vsprintf_s(s, ss, f, a) vsnprintf(s, ss, f, a)
#endif

void splitString(const std::string & str, std::vector<std::string> & result);
void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

struct BoxAtlasElem;
class BuiltinShaders;
class FontCacheElem;
struct GxTexture;
class MsdfFontCache;
struct TextureAtlas;

//

#if ENABLE_UTF8_SUPPORT
	typedef uint32_t GlyphCode;
#else
	typedef char GlyphCode;
#endif

//

/*
MouseData contains the mouse data for a Window (obviously). but less obvious is the way in which mouse events are processed. mouse events are processed over
multiple frames if we have to. due to the way the mouse API works (mouse.wentDown, mouse.wentUp) we need to process cases where both the mouse down and mouse up
event arrive during a framework.process call. this may happen for instance when using a touch pad which simulates mouse events during 'soft touches' (touches
where not physically pressing down a button or a surface). since there is no physical action, and the mouse events are simulated when the finger is released,
without moving, from the touch pad, there is no delay between the mouse down and mouse up events. in these cases, we may wish to process both events separately,
as otherwise the behavior of mouse.wentDown and mouse.wentUp doesn't yield the expected behavior

consider the following sequence of events:

	framework.process()
		(event) MouseDown -> true
		(event) MouseDown -> false
 
	if (mouse.wentDown(..))
	{
		// code doesn't get here
	}

wentDown returns false here, as isDown is already set to false. so we 'missed' the event

now consider the case where we spead event processing over multiple framework.process calls:

	framework.process()
		(event) MouseDown -> true
 
	if (mouse.wentDown(..))
	{
		// do something useful here
	}
 
 	framework.process()
		(event) MouseDown -> false

it will detect the mouse down event, and during the next process, mouse.wentUp will be true
*/

class MouseData
{
public:
	bool mouseDown[BUTTON_MAX];
	bool mouseChange[BUTTON_MAX];
	bool hasOldMousePosition;
	
	int mouseX;
	int mouseY;
	int mouseDx;
	int mouseDy;
	int mouseScrollY;
	int oldMouseX;
	int oldMouseY;
	
#if ENABLE_EVENTS_WORKAROUND
	SDL_Event events[32];
	int numEvents = 0;
#else
	std::vector<SDL_Event> events;
#endif

	MouseData()
	{
		memset(mouseDown, 0, sizeof(mouseDown));
		memset(mouseChange, 0, sizeof(mouseChange));
		hasOldMousePosition = false;
		
		mouseX = 0;
		mouseY = 0;
		mouseDx = 0;
		mouseDy = 0;
		mouseScrollY = 0;
		oldMouseX = 0;
		oldMouseY = 0;
	}
	
	void addEvent(const SDL_Event & e)
	{
	#if ENABLE_EVENTS_WORKAROUND
		events[numEvents++] = e;
	#else
		events.push_back(e);
	#endif
	}
	
	void processEvents()
	{
	#if !ENABLE_EVENTS_WORKAROUND
		const int numEvents = events.size();
	#endif
	
		if (numEvents == 0)
			return;

		size_t eventIndex = 0;
		
		bool stop = false;
		
		while (stop == false)
		{
			const SDL_Event & e = events[eventIndex];
			
			if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				const int index = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : -1;
				
				if (index >= 0)
				{
					mouseDown[index] = true;
					mouseChange[index] = true;
					stop = true;
				}
			}
			else if (e.type == SDL_MOUSEBUTTONUP)
			{
				const int index = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : -1;
				
				if (index >= 0)
				{
					mouseDown[index] = false;
					mouseChange[index] = true;
					stop = true;
				}
			}
			else if (e.type == SDL_MOUSEMOTION)
			{
				mouseX = e.motion.x;
				mouseY = e.motion.y;
			}
			else
			{
				Assert(false);
			}
			
			eventIndex++;
			
			if (eventIndex == numEvents)
				stop = true;
		}
		
		if (eventIndex == numEvents)
		#if ENABLE_EVENTS_WORKAROUND
			numEvents = 0;
		#else
			events.clear();
		#endif
		else
		{
		#ifdef WIN32
			std::reverse(events, events + numEvents);
			for (int i = 0; i < eventIndex; ++i)
				numEvents--;
			std::reverse(events, events + numEvents);
#if 0
			printf("size: %d\n", (int)events.size());
			for (int i = 0; i < eventIndex; ++i)
			{
				auto itr = events.begin();
				events.erase(itr);
			}
#endif
		#else
			events.erase(events.begin(), events.begin() + eventIndex);
		#endif
		}
	}
	
	void beginProcess()
	{
		memset(mouseChange, 0, sizeof(mouseChange));
		
		mouseDx = 0;
		mouseDy = 0;
		mouseScrollY = 0;
		
		oldMouseX = mouseX;
		oldMouseY = mouseY;
	}
	
	void endProcess()
	{
		processEvents();
		
		//
		
		if (hasOldMousePosition)
		{
			mouseDx = mouseX - oldMouseX;
			mouseDy = mouseY - oldMouseY;
		}
		else
		{
			if (mouseX != oldMouseX || mouseY != oldMouseY)
			{
				hasOldMousePosition = true;
			}
			
			mouseDx = 0;
			mouseDy = 0;
		}
	}
};

class WindowData
{
public:
	void beginProcess()
	{
		quitRequested = false;
		
		keyChangeCount = 0;
		keyRepeatCount = 0;
		
		mouseData.beginProcess();
	}
	
	void endProcess()
	{
		mouseData.endProcess();
	}
	
	void makeActive() const
	{
		framework.windowIsActive = isActive;
		
		mouse.x = mouseData.mouseX;
		mouse.y = mouseData.mouseY;
		mouse.dx = mouseData.mouseDx;
		mouse.dy = mouseData.mouseDy;
		mouse.scrollY = mouseData.mouseScrollY;
	}
	
	bool isActive;
	bool quitRequested;
	int keyDown[256];
	int keyDownCount;
	int keyChange[256];
	int keyChangeCount;
	int keyRepeat[256];
	int keyRepeatCount;
	
	MouseData mouseData;
};

//

struct DepthTestInfo
{
	bool testEnabled;
	DEPTH_TEST test;
	bool writeEnabled;
};

struct DepthBiasInfo
{
	float depthBias;
	float slopeScale;
};

struct AlphaToCoverageInfo
{
	bool enabled;
};

//

struct CullModeInfo
{
	CULL_MODE mode;
	CULL_WINDING winding;
};

//

class Globals
{
public:
	Globals()
	{
		memset(this, 0, sizeof(Globals));
		blendMode = BLEND_ALPHA;
		colorMode = COLOR_MUL;
		colorPost = POST_NONE;
		fontMode = FONT_BITMAP;
		colorClamp = true;
		transform = TRANSFORM_SCREEN;
		transformScreen.MakeIdentity();
		transform2d.MakeIdentity();
		transform3d.MakeIdentity();
		gxShaderIsDirty = true;
		depthTest = DEPTH_LESS;
		depthTestWriteEnabled = true;
		frontStencilState = StencilState();
		backStencilState = StencilState();
	}
	
	Window * mainWindow;
	Window * currentWindow;
	SDL_GLContext glContext;
	int displaySize[2]; // size as passed to init
#if ENABLE_PROFILING
	Remotery * rmt;
#endif
#if USE_FREETYPE
	FT_Library freeType;
#endif
	int resourceVersion;
	BLEND_MODE blendMode;
	COLOR_MODE colorMode;
	COLOR_POST colorPost;
	bool lineSmoothEnabled;
	bool wireframeEnabled;
	FONT_MODE fontMode;
	Color color;
	bool colorClamp;
	bool depthTestEnabled;
	DEPTH_TEST depthTest;
	bool depthTestWriteEnabled;
	float depthBias;
	float depthBiasSlopeScale;
	bool alphaToCoverageEnabled;
	bool stencilEnabled;
	StencilState frontStencilState;
	StencilState backStencilState;
	CULL_MODE cullMode;
	CULL_WINDING cullWinding;
	GRADIENT_TYPE hqGradientType;
	Mat4x4 hqGradientMatrix;
	Color hqGradientColor1;
	Color hqGradientColor2;
	COLOR_MODE hqGradientColorMode;
	float hqGradientBias;
	float hqGradientScale;
	bool hqTextureEnabled;
	Mat4x4 hqTextureMatrix;
	bool hqUseScreenSize;
	FontCacheElem * font;
	bool isInTextBatch;
	MsdfFontCacheElem * fontMSDF;
	bool isInTextBatchMSDF;
	int xinputGamepadIdx;
	SDL_Joystick * joystick[GAMEPAD_MAX];
	ShaderBase * shader;
	TRANSFORM transform;
	Mat4x4 transformScreen;
	Mat4x4 transform2d;
	Mat4x4 transform3d;
	bool gxShaderIsDirty;
	BuiltinShaders * builtinShaders;
	char shaderOutputs[32];
	
	struct DebugDraw
	{
		static const int kMaxLines = 32;
		static const int kMaxLineSize = 128;
		
		struct
		{
			FontCacheElem * font;
			Color color;
			float x;
			float y;
			int size;
			float alignX;
			float alignY;
			char text[kMaxLineSize];
		} lines[kMaxLines];
		int numLines;
	} debugDraw;
};

//

#if USE_STBFONT || ENABLE_MSDF_FONTS

class StbFont
{
public:
	uint8_t * buffer;
	int bufferSize;
	
	stbtt_fontinfo fontInfo;
	
	StbFont();
	~StbFont();
	
	bool load(const char * filename);
	void free();
};

#endif

//

class TextureCacheElem
{
public:
	std::string name;
	GxTexture * textures;
	int sx;
	int sy;
	int gridSx;
	int gridSy;
	bool mipmapped;
	
	TextureCacheElem();
	void free();
	void load(const char * filename, int gridSx, int gridSy, bool mipmapped);
	void reload();
};

class TextureCache
{
public:
	class Key
	{
	public:
		std::string name;
		int gridSx;
		int gridSy;
		bool mipmapped;
		
		inline bool operator<(const Key & other) const
		{
			if (name != other.name)
				return name < other.name;
			if (gridSx != other.gridSx)
				return gridSx < other.gridSx;
			if (gridSy != other.gridSy)
				return gridSy < other.gridSy;
			if (mipmapped != other.mipmapped)
				return mipmapped < other.mipmapped;
			return false;
		}
	};
	typedef std::map<Key, TextureCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	TextureCacheElem & findOrCreate(const char * name, int gridSx, int gridSy, bool mipmapped);
};

//

class AnimCacheElem
{
public:
	class AnimTrigger
	{
	public:
		enum Event
		{
			OnEnter,
			OnLeave
		};
		
		Event event;
		std::string action;
		Dictionary args;
	};

	class Anim
	{
	public:
		std::string name;
		int firstCell;
		int numFrames;
		float frameRate;
		int pivot[2];
		bool loop;
		int loopStart;
		
		std::vector< std::vector<AnimTrigger> > frameTriggers;
		
		Anim()
		{
			firstCell = 0;
			numFrames = 1;
			frameRate = 0.f;
			pivot[0] = pivot[1] = 0;
			loop = false;
			loopStart = 0;
		}
	};
	
	typedef std::map<std::string, Anim> AnimMap;
	
	bool m_hasSheet;
	int m_gridSize[2];
	int m_pivot[2];
	float m_scale;
	AnimMap m_animMap;
	
	AnimCacheElem();
	void free();
	void load(const char * filename);
	int getVersion() const;
};

class AnimCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, AnimCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	AnimCacheElem & findOrCreate(const char * name);
};

//

namespace spriter
{
	class Scene;
}

class SpriterCacheElem
{
public:
	spriter::Scene * m_scene;

	SpriterCacheElem();
	void free();
	void load(const char * filename);
};

class SpriterCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, SpriterCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	SpriterCacheElem & findOrCreate(const char * name);
};

//

class SoundCacheElem
{
public:
#if FRAMEWORK_USE_OPENAL
	ALuint buffer;
#else
	void * buffer;
#endif
	
	SoundCacheElem();
	void free();
	void load(const char * filename);
};

class SoundCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, SoundCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	SoundCacheElem & findOrCreate(const char * name);
};

//

class FontCacheElem
{
public:
#if USE_STBFONT
	StbFont * font;
#elif USE_FREETYPE
	FT_Face face;
#endif
#if USE_GLYPH_ATLAS
	TextureAtlas * textureAtlas;
#endif
	
	FontCacheElem();
	void free();
	void load(const char * filename);
};

class FontCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, FontCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	FontCacheElem & findOrCreate(const char * name);
};

//

class GlyphCacheElem
{
public:
#if USE_STBFONT
	int y;
	int sx;
	int sy;

	int advance;
	int lsb;
#elif USE_FREETYPE
	FT_GlyphSlotRec g;
#endif

#if USE_GLYPH_ATLAS
	BoxAtlasElem * textureAtlasElem;
#else
	GLuint texture;
#endif
};

class GlyphCache
{
public:
	class Key
	{
	public:
	#if USE_STBFONT
		const StbFont * font;
	#elif USE_FREETYPE
		FT_Face face;
	#endif
		int size;
		int c;
		
		inline bool operator<(const Key & other) const
		{
		#if USE_STBFONT
			if (font != other.font)
				return font < other.font;
		#elif USE_FREETYPE
			if (face != other.face)
				return face < other.face;
		#endif

			if (size != other.size)
				return size < other.size;
			return c < other.c;
		}
	};
	
	typedef std::map<Key, GlyphCacheElem> Map;
	
	Map m_map;
	
	void clear();
#if USE_STBFONT
	GlyphCacheElem & findOrCreate(const StbFont * font, int size, int c);
#elif USE_FREETYPE
	GlyphCacheElem & findOrCreate(FT_Face face, int size, int c);
#endif
};

//

#if ENABLE_MSDF_FONTS

namespace msdfgen
{
	class Shape;
}

class MsdfGlyphCacheElem
{
public:
	BoxAtlasElem * textureAtlasElem;
	int y;
	int sx;
	int sy;
	float scale;
	int advance;
	int lsb;
	bool isInitialized;
	
	MsdfGlyphCacheElem();
};

class MsdfGlyphCache
{
public:
	typedef std::map<int, MsdfGlyphCacheElem> Map;
	
	bool m_isLoaded;
	StbFont m_font;
	TextureAtlas * m_textureAtlas;
	
	Map m_map;
	
	MsdfGlyphCache();
	~MsdfGlyphCache();
	
	void free();
	void allocTextureAtlas();
	void load(const char * filename);
	const MsdfGlyphCacheElem & findOrCreate(int c);
	
	bool stbGlyphToMsdfShape(const int codePoint, msdfgen::Shape & shape);
	void makeGlyph(const int codepoint, MsdfGlyphCacheElem & glyph);
	
	bool saveCache(const char * filename) const;
	bool loadCache(const char * filename);
};

//

class MsdfFontCacheElem
{
public:
	std::string m_filename;
	MsdfGlyphCache * m_glyphCache;
	
	MsdfFontCacheElem();
	void free();
	void load(const char * filename);
};

class MsdfFontCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, MsdfFontCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	MsdfFontCacheElem & findOrCreate(const char * name);
};

#endif

//

template <typename Type, int kMaxStackSize>
struct Stack
{
	Type stack[kMaxStackSize];
	int stackSize = 0;
	
	Stack()
	{
	}
	
	Stack(const Type defaultValue)
	{
		push(defaultValue);
	}
	
	void push(Type value)
	{
		fassert(stackSize < kMaxStackSize);
		stack[stackSize++] = value;
	}

	void pop()
	{
		fassert(stackSize > 0);
		--stackSize;
		memset(&stack[stackSize], 0, sizeof(Type));
	}
	
	Type popValue()
	{
		fassert(stackSize >= 1);
		Type value = stack[stackSize - 1];
		
		pop();
		
		return value;
	}
};

//

class BuiltinShader
{
	std::string filename;
	bool shaderIsInit;
	Shader shader;
	
public:
	BuiltinShader(const char * _filename)
		: filename(_filename)
		, shaderIsInit(false)
		, shader()
	{
	#if !defined(DEBUG)
		get();
	#endif
	}
	
	Shader & get()
	{
		if (shaderIsInit == false)
		{
			shaderIsInit = true;
			
			const std::string vs = std::string(filename) + ".vs";
			const std::string ps = std::string(filename) + ".ps";
			
			shader.load(filename.c_str(), vs.c_str(), ps.c_str(), "c");
		}
		
		return shader;
	}
};

class BuiltinShaders
{
public:
	BuiltinShaders();
	
	BuiltinShader gaussianBlurH;
	BuiltinShader gaussianBlurV;
	ShaderBuffer gaussianKernelBuffer;
	
	BuiltinShader colorMultiply;
	BuiltinShader colorTemperature;
	
	BuiltinShader textureSwizzle;
	
	BuiltinShader threshold;
	BuiltinShader thresholdValue;
	
	BuiltinShader hqLine;
	BuiltinShader hqFilledTriangle;
	BuiltinShader hqFilledCircle;
	BuiltinShader hqFilledRect;
	BuiltinShader hqFilledRoundedRect;
	BuiltinShader hqStrokeTriangle;
	BuiltinShader hqStrokedCircle;
	BuiltinShader hqStrokedRect;
	BuiltinShader hqStrokedRoundedRect;
	
	BuiltinShader hqShadedTriangle;
	
	BuiltinShader msdfText;
	
	BuiltinShader bitmappedText;
};

//

struct ShaderOutput
{
	char name;
	std::string outputType;
	std::string outputName;
};

const ShaderOutput * findShaderOutput(const char name);

//

class ScopedLoadTimer
{
public:
#if defined(DEBUG)
	const char * m_filename;
	uint64_t m_startTime;

	ScopedLoadTimer(const char * filename);
	~ScopedLoadTimer();
#else
	ScopedLoadTimer(const char * filename)
	{
	}
#endif
};

//

extern Globals globals;

extern TextureCache g_textureCache;
#if ENABLE_OPENGL
extern ShaderCache g_shaderCache;
#if ENABLE_OPENGL_COMPUTE_SHADER
extern ComputeShaderCache g_computeShaderCache;
#endif
#endif
extern AnimCache g_animCache;
extern SpriterCache g_spriterCache;
extern SoundCache g_soundCache;
extern FontCache g_fontCache;
extern MsdfFontCache g_fontCacheMSDF;
extern GlyphCache g_glyphCache;

extern std::vector<ShaderOutput> g_shaderOutputs;
