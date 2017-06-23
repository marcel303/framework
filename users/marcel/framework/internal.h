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
#include <map>
#include <OpenAL/al.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string>
#include "framework.h"
#include "stb_truetype.h"

#define MAX_MIDI_KEYS 256

#define USE_GLYPH_ATLAS 1

#define ENABLE_MSDF_FONTS 1

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
#define sprintf_s(s, ss, f, ...) sprintf(s, f, __VA_ARGS__)
#define vsprintf_s(s, ss, f, a) vsprintf(s, f, a)
#endif

void splitString(const std::string & str, std::vector<std::string> & result);
void splitString(const std::string & str, std::vector<std::string> & result, char c);

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	void __stdcall debugOutputGL(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, GLvoid*);
#endif

//

struct BoxAtlasElem;
class BuiltinShaders;
class FontCacheElem;
class MsdfFontCache;
struct TextureAtlas;

//

class Globals
{
public:
	Globals()
	{
		memset(this, 0, sizeof(Globals));
		blendMode = BLEND_ALPHA;
		colorMode = COLOR_MUL;
		fontMode = FONT_BITMAP;
		transform = TRANSFORM_SCREEN;
		transformScreen.MakeIdentity();
		transform2d.MakeIdentity();
		transform3d.MakeIdentity();
		gxShaderIsDirty = true;
	}
	
	SDL_Window * window;
	SDL_GLContext glContext;
	int displaySize[2];
	int actualDisplaySize[2];
	int drawableOffset[2];
#if ENABLE_PROFILING
	Remotery * rmt;
#endif
	FT_Library freeType;
	int resourceVersion;
	BLEND_MODE blendMode;
	COLOR_MODE colorMode;
	FONT_MODE fontMode;
	Color color;
	Gradient gradient;
	FontCacheElem * font;
	bool isInTextBatch;
	MsdfFontCacheElem * fontMSDF;
	bool isInTextBatchMSDF;
	bool mouseDown[BUTTON_MAX];
	bool mouseChange[BUTTON_MAX];
	bool hasOldMousePosition;
	int keyDown[256];
	int keyDownCount;
	int keyChange[256];
	int keyChangeCount;
	int keyRepeat[256];
	int keyRepeatCount;
	int xinputGamepadIdx;
	bool midiIsSet[MAX_MIDI_KEYS];
	bool midiIsSetAsync[MAX_MIDI_KEYS];
	bool midiDown[MAX_MIDI_KEYS];
	bool midiDownAsync[MAX_MIDI_KEYS];
	bool midiChange[MAX_MIDI_KEYS];
	bool midiChangeAsync[MAX_MIDI_KEYS];
	float midiValue[MAX_MIDI_KEYS];
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
		kSp_Texture,
		kSp_Params,
		kSp_MAX
	};

	std::string name;
	std::string vs;
	std::string ps;

	GLuint program;

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
	ALuint buffer;
	
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
	FT_Face face;
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
	FT_GlyphSlotRec g;
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
		FT_Face face;
		int size;
		int c;
		
		inline bool operator<(const Key & other) const
		{
			if (face != other.face)
				return face < other.face;
			if (size != other.size)
				return size < other.size;
			return c < other.c;
		}
	};
	
	typedef std::map<Key, GlyphCacheElem> Map;
	
	Map m_map;
	
	void clear();
	GlyphCacheElem & findOrCreate(FT_Face face, int size, int c);
};

//

#if ENABLE_MSDF_FONTS

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

#if ENABLE_UTF8_SUPPORT
	typedef uint32_t MsdfGlyphCode;
#else
	typedef char MsdfGlyphCode;
#endif

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

class UiCacheElem
{
public:
	typedef std::map<std::string, Dictionary> Map;
	
	Map map;
	
	void free();
	void load(const char * filename);
};

class UiCache
{
public:
	typedef std::map<std::string, UiCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	UiCacheElem & findOrCreate(const char * filename);
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

class BuiltinShaders
{
public:
	BuiltinShaders();
	
	Shader gaussianBlurH;
	Shader gaussianBlurV;
	Shader colorMultiply;
	Shader colorTemperature;
	
	Shader hqLine;
	Shader hqFilledTriangle;
	Shader hqFilledCircle;
	Shader hqFilledRect;
	Shader hqFilledRoundedRect;
	Shader hqStrokeTriangle;
	Shader hqStrokedCircle;
	Shader hqStrokedRect;
	
	Shader msdfText;
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
extern UiCache g_uiCache;
