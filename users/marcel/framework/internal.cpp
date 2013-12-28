#include "audio.h"
#include "internal.h"

Globals g_globals;
TextureCache g_textureCache;
SoundCache g_soundCache;
FontCache g_fontCache;
GlyphCache g_glyphCache;

// -----
	
TextureCacheElem::TextureCacheElem()
{
	texture = 0;
}

void TextureCacheElem::free()
{
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}
}

void TextureCacheElem::load(const char * filename)
{
	free();
	
	glGenTextures(1, &texture);
	
	if (texture != 0)
	{
		// todo: load texture data
		
		// todo: tex image
		
		log("loaded %s", filename);
	}
}

void TextureCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void TextureCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

TextureCacheElem & TextureCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		TextureCacheElem elem;
		
		elem.load(name);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

SoundCacheElem::SoundCacheElem()
{
	buffer = 0;
}

void SoundCacheElem::free()
{
	if (buffer != 0)
	{
		g_soundPlayer.stopSoundsForBuffer(buffer);
		
		alDeleteBuffers(1, &buffer);
		buffer = 0;
		g_soundPlayer.checkError();
	}
}

void SoundCacheElem::load(const char * filename)
{
	free();
	
	SoundData * soundData = loadSound(filename);
	
	if (soundData != 0)
	{
		alGenBuffers(1, &buffer);
		g_soundPlayer.checkError();
		
		if (buffer != 0)
		{
			ALenum bufferFormat;
			
			if (soundData->channelCount == 1)
			{
				if (soundData->channelSize == 1)
					bufferFormat = AL_FORMAT_MONO8;
				else
					bufferFormat = AL_FORMAT_MONO16;
			}
			else
			{
				if (soundData->channelSize == 1)
					bufferFormat = AL_FORMAT_STEREO8;
				else
					bufferFormat = AL_FORMAT_STEREO16;
			}
			
			ALuint bufferSize = soundData->sampleCount * soundData->channelCount * soundData->channelSize;
			ALuint bufferSampleRate = soundData->sampleRate;
			
			log("WAVE: buffer=%u, bufferFormat=%d, samleData=%p, bufferSize=%d, sampleRate=%d",
				buffer,
				bufferFormat,
				soundData->sampleData,
				bufferSize,
				soundData->sampleRate);

			alBufferData(buffer, bufferFormat, soundData->sampleData, bufferSize, bufferSampleRate);
			g_soundPlayer.checkError();
			
			log("loaded %s", filename);
		}
		else
		{
			logError("failed to create OpenAL buffer");
		}
		
		delete soundData;
	}
	else
	{
		logError("failed to load sound: %s", filename);
	}
}

void SoundCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void SoundCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

SoundCacheElem & SoundCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		SoundCacheElem elem;
		
		elem.load(name);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

FontCacheElem::FontCacheElem()
{
	face = 0;
}

void FontCacheElem::free()
{
	if (face != 0)
	{
		if (FT_Done_Face(face) != 0)
		{
			logError("failed to free face");
		}
		
		face = 0;
	}
}

void FontCacheElem::load(const char * filename)
{
	free();
	
	if (FT_New_Face(g_globals.g_freeType, filename, 0, &face) != 0)
	{
		logError("unable to open font");
		face = 0;
	}
	else
	{
		log("loaded %s", filename);
	}
}

void FontCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void FontCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

FontCacheElem & FontCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		FontCacheElem elem;
		
		elem.load(name);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

void GlyphCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		if (i->second.texture != 0)
		{
			glDeleteTextures(1, &i->second.texture);
		}
	}
	
	m_map.clear();
}

GlyphCacheElem & GlyphCache::findOrCreate(FT_Face face, int size, char c)
{
	Key key;
	key.face = face;
	key.size = size;
	key.c = c;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		// lookup failed. render the glyph and add the new element to the cache
		
		GlyphCacheElem elem;
		
		FT_Set_Pixel_Sizes(face, 0, size);
		
		if (FT_Load_Char(face, c, FT_LOAD_RENDER) == 0)
		{
			// capture current OpenGL states before we change them
			
			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			GLint restoreUnpack;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
			
			// create texture and copy image data
			
			elem.g = *face->glyph;
			glGenTextures(1, &elem.texture);
			
			glBindTexture(GL_TEXTURE_2D, elem.texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_ALPHA,
				elem.g.bitmap.width,
				elem.g.bitmap.rows,
				0,
				GL_ALPHA,
				GL_UNSIGNED_BYTE,
				elem.g.bitmap.buffer);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			// restore previous OpenGL states
			
			glBindTexture(GL_TEXTURE_2D, restoreTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
		}
		else
		{
			// failed to render the glyph. return the NULL texture handle
			
			logError("failed to render glyph");
			elem.texture = 0;
		}
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		printf("added glyph cache element. face=%p, size=%d, character=%c, texture=%u. count=%d\n", face, size, c, elem.texture, (int)m_map.size());
		
		return i->second;
	}
}
