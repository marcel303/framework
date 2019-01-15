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

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>
#include <map>
#include <SDL2/SDL.h>
#include <string>
#include "framework.h"
#include "stb_truetype.h"

#if FRAMEWORK_USE_OPENAL
	#include <OpenAL/al.h>
#endif

#define MAX_MIDI_KEYS 256

#if !defined(USE_GLYPH_ATLAS)
	#if !USE_LEGACY_OPENGL
		#define USE_GLYPH_ATLAS 1
	#else
		#define USE_GLYPH_ATLAS 0 // cannot use glyph cache, as it uses R8 texture storage
	#endif
#endif

#define USE_STBFONT 0

#if !defined(ENABLE_MSDF_FONTS)
	#if !USE_LEGACY_OPENGL
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

#ifndef WIN32
static int fopen_s(FILE ** file, const char * filename, const char * mode)
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
	
	std::vector<SDL_Event> events;
	
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
		events.push_back(e);
	}
	
	void processEvents()
	{
		if (events.empty())
			return;
		
		const size_t numEvents = events.size();
		
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
			events.clear();
		else
		{
		#ifdef WIN32
			std::reverse(events.begin(), events.end());
			for (int i = 0; i < eventIndex; ++i)
				events.pop_back();
			std::reverse(events.begin(), events.end());
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
	}
	
	Window * mainWindow;
	SDL_Window * currentWindow;
	WindowData * currentWindowData;
	SDL_GLContext glContext;
	int displaySize[2]; // size as passed to init
#if ENABLE_PROFILING
	Remotery * rmt;
#endif
	FT_Library freeType;
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
	bool midiIsSet[MAX_MIDI_KEYS];
	bool midiIsSetAsync[MAX_MIDI_KEYS];
	bool midiDown[MAX_MIDI_KEYS];
	bool midiDownAsync[MAX_MIDI_KEYS];
	bool midiChange[MAX_MIDI_KEYS];
	bool midiChangeAsync[MAX_MIDI_KEYS];
	float midiValue[MAX_MIDI_KEYS];
	SDL_Joystick * joystick[GAMEPAD_MAX];
	ShaderBase * shader;
	TRANSFORM transform;
	Mat4x4 transformScreen;
	Mat4x4 transform2d;
	Mat4x4 transform3d;
	bool gxShaderIsDirty;
	BuiltinShaders * builtinShaders;
	
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

struct VsInput
{
	int id;
	int components;
	int type;
	bool normalize;
	int offset;
};

void bindVsInputs(const VsInput * vsInputs, int numVsInputs, int stride);

//

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

//

class TextureCacheElem
{
public:
	std::string name;
	GLuint * textures;
	int sx;
	int sy;
	int gridSx;
	int gridSy;
	
	TextureCacheElem();
	void free();
	void load(const char * filename, int gridSx, int gridSy);
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
		
		inline bool operator<(const Key & other) const
		{
			if (name != other.name)
				return name < other.name;
			if (gridSx != other.gridSx)
				return gridSx < other.gridSx;
			if (gridSy != other.gridSy)
				return gridSy < other.gridSy;
			return false;
		}
	};
	typedef std::map<Key, TextureCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	TextureCacheElem & findOrCreate(const char * name, int gridSx, int gridSy);
};

//

class ShaderCacheElem
{
public:
	enum ShaderParam
	{
		kSp_ModelViewMatrix,
		kSp_ModelViewProjectionMatrix,
		kSp_ProjectionMatrix,
		kSp_SkinningMatrices,
		kSp_Texture,
		kSp_Params,
		kSp_ShadingParams,
		kSp_GradientInfo,
		kSp_GradientMatrix,
		kSp_TextureMatrix,
		kSp_MAX
	};

	std::string name;
	std::string vs;
	std::string ps;
	
	GLuint program;
	
	int version;
	std::vector<std::string> errorMessages;

	struct
	{
		GLint index;

		void set(GLint index)
		{
			this->index = index;
		}
	} params[kSp_MAX];

	ShaderCacheElem();
	void free();
	void load(const char * name, const char * filenameVs, const char * filenamePs);
	void reload();
};

class ShaderCache
{
public:
	typedef std::map<std::string, ShaderCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	ShaderCacheElem & findOrCreate(const char * name, const char * filenameVs, const char * filenamePs);
};

//

class ComputeShaderCacheElem
{
public:
	std::string name;
	int groupSx;
	int groupSy;
	int groupSz;

	GLuint program;
	
	int version;
	std::vector<std::string> errorMessages;

	ComputeShaderCacheElem();
	void free();
	void load(const char * filename, const int groupSx, const int groupSy, const int groupSz);
	void reload();
};

class ComputeShaderCache
{
public:
	typedef std::map<std::string, ComputeShaderCacheElem> Map;

	Map m_map;

	void clear();
	void reload();
	ComputeShaderCacheElem & findOrCreate(const char * filename, const int groupSx, const int groupSy, const int groupSz);
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
#else
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
#else
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
	#else
		FT_Face face;
	#endif
		int size;
		int c;
		
		inline bool operator<(const Key & other) const
		{
		#if USE_STBFONT
			if (font != other.font)
				return font < other.font;
		#else
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
#else
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

class FileReader
{
public:
	FileReader()
	{
		file = 0;
	}
	
	~FileReader()
	{
		close();
	}
	
	bool open(const char * filename, bool textMode)
	{
		fopen_s(&file, filename, textMode ? "rt" : "rb");
		
		return file != 0;
	}
	
	void close()
	{
		if (file != 0)
		{
			fclose(file);
			file = 0;
		}
	}
	
	template <typename T>
	bool read(T & dst)
	{
		return fread(&dst, sizeof(dst), 1, file) == 1;
	}
	
	bool read(void * dst, int numBytes)
	{
		return fread(dst, numBytes, 1, file) == 1;
	}
	
	bool read(std::string & dst)
	{
		char line[1024];
		if (fgets(line, sizeof(line), file) == 0)
			return false;
		else
		{
			dst = line;
			return true;
		}
	}
	
	bool skip(int numBytes)
	{
		return fseek(file, numBytes, SEEK_CUR) == 0;
	}
	
	FILE * file;
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
	}
	
	Shader & get()
	{
		if (shaderIsInit == false)
		{
			shaderIsInit = true;
			
			const std::string vs = std::string(filename) + ".vs";
			const std::string ps = std::string(filename) + ".ps";
			
			shader.load(filename.c_str(), vs.c_str(), ps.c_str());
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
};

//

extern Globals globals;

extern TextureCache g_textureCache;
extern ShaderCache g_shaderCache;
extern ComputeShaderCache g_computeShaderCache;
extern AnimCache g_animCache;
extern SpriterCache g_spriterCache;
extern SoundCache g_soundCache;
extern FontCache g_fontCache;
extern MsdfFontCache g_fontCacheMSDF;
extern GlyphCache g_glyphCache;
