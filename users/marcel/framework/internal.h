#pragma once

#include <OpenAL/al.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <string>
#include "framework.h"

class Globals
{
public:
	Globals()
	{
		memset(this, 0, sizeof(Globals));
	}
	
	int g_displaySize[2];
	FT_Library g_freeType;
	int g_resourceVersion;
	BLEND_MODE g_blendMode;
	Color g_color;
	FontCacheElem * g_font;
	int g_mouseX;
	int g_mouseY;
	bool g_mouseDown[2];
	bool g_keyDown[SDLK_LAST];
};

//

class TextureCacheElem
{
public:
	GLuint texture;
	int sx;
	int sy;
	
	TextureCacheElem();
	void free();
	void load(const char * filename);
};

class TextureCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, TextureCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	TextureCacheElem & findOrCreate(const char * name);
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
		
		std::vector< std::vector<AnimTrigger> > frameTriggers;
		
		Anim()
		{
			firstCell = 0;
			numFrames = 1;
			frameRate = 0.f;
			pivot[0] = pivot[1] = 0;
			loop = false;
		}
	};
	
	typedef std::map<std::string, Anim> AnimMap;
	
	int m_animCellCount[2];
	int m_pivot[2];
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
	GLuint texture;
};

class GlyphCache
{
public:
	class Key
	{
	public:
		FT_Face face;
		int size;
		char c;
		
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
	GlyphCacheElem & findOrCreate(FT_Face face, int size, char c);
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
	
	bool open(const char * filename)
	{
		file = fopen(filename, "rb");
		
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

extern Globals g_globals;
extern TextureCache g_textureCache;
extern AnimCache g_animCache;
extern SoundCache g_soundCache;
extern FontCache g_fontCache;
extern GlyphCache g_glyphCache;
