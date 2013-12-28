#include "audio.h"
#include "image.h"
#include "internal.h"

Globals g_globals;
TextureCache g_textureCache;
AnimCache g_animCache;
SoundCache g_soundCache;
FontCache g_fontCache;
GlyphCache g_glyphCache;

// -----
	
TextureCacheElem::TextureCacheElem()
{
	texture = 0;
	sx = sy = 0;
}

void TextureCacheElem::free()
{
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
		sx = sy = 0;
	}
}

void TextureCacheElem::load(const char * filename)
{
	free();
	
	ImageData * imageData = loadImage(filename);
	
	if (!imageData)
	{
		logError("failed to load %s", filename);
	}
	else
	{
		glGenTextures(1, &texture);
		
		if (texture != 0)
		{
			// capture current OpenGL states before we change them
			
			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			GLint restoreUnpack;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
			
			// copy image data
						
			glBindTexture(GL_TEXTURE_2D, texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGBA,
				imageData->sx,
				imageData->sy,
				0,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				imageData->imageData);
			
		#if 1
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		#else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		#endif
			
			// restore previous OpenGL states
			
			glBindTexture(GL_TEXTURE_2D, restoreTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
			
			sx = imageData->sx;
			sy = imageData->sy;
			
			log("loaded %s", filename);
		}
		
		delete imageData;
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

AnimCacheElem::AnimCacheElem()
{
	free();
}

void AnimCacheElem::free()
{
	m_animCellCount[0] = m_animCellCount[1] = 1;
	
	m_animMap.clear();
}

static bool isWhite(char c)
{
	return c == '\t' || c == ' ' || c == '\r' || c == '\n';
}

static void splitString(std::string & str, std::vector<std::string> & result)
{
	int start = -1;
	
	for (size_t i = 0; i <= str.size(); ++i)
	{
		const char c = i < str.size() ? str[i] : ' ';
		
		if (start == -1)
		{
			// found start
			if (!isWhite(c))
				start = i;
		}
		else if (isWhite(c))
		{
			// found end
			result.push_back(str.substr(start, i - start));
			start = -1;
		}
	}
}

void AnimCacheElem::load(const char * filename)
{
	free();
	
	FileReader r;
	
	if (!r.open(filename))
	{
		//logError("failed to open %s", filename);
	}
	else
	{	
		Anim * currentAnim = 0;
		
		std::string line;
		
		while (r.read(line))
		{
			// format: <name> <key>:<value> <key:value> <key..
							
			std::vector<std::string> parts;
			splitString(line, parts);
			
			if (parts.size() == 0 || parts[0][0] == '#')
			{
				// empty line or comment
				continue;
			}
				
			if (parts.size() == 1)
			{
				logError("missing parameters: %s (%s)", line.c_str(), parts[0].c_str());
				continue;
			}
			
			const std::string section = parts[0];
			Dictionary args;
			
			for (size_t i = 1; i < parts.size(); ++i)
			{
				const size_t separator = parts[i].find(':');
				
				if (separator == std::string::npos)
				{
					logError("incorrect key:value syntax: %s (%s)", line.c_str(), parts[i].c_str());
					continue;
				}
				
				const std::string key = parts[i].substr(0, separator);
				const std::string value = parts[i].substr(separator + 1, parts[i].size() - separator - 1);
				
				if (key.size() == 0 || value.size() == 0)
				{
					logError("incorrect key:value syntax: %s (%s)", line.c_str(), parts[i].c_str());
					continue;
				}
				
				if (args.contains(key.c_str()))
				{
					logError("duplicate key: %s (%s)", line.c_str(), key.c_str());
					continue;
				}
				
				//log("added: %s:%s", key.c_str(), value.c_str());
				
				args.setString(key.c_str(), value.c_str());
			}
			
			// sheet pivot_x:0 pivot_y:0 cells_x:4 cells_y:8
			// animation name:walk grid_x:0 grid_y:0 frames:12 rate:4 loop:0 pivot_x:2 pivot_y:2
			// trigger frame:3 action:sound sound:test.wav
			
			if (section == "sheet")
			{
				m_animCellCount[0] = args.getInt("grid_sx", 1);
				m_animCellCount[1] = args.getInt("grid_sy", 1);
				m_pivot[0] = args.getInt("pivot_x", 0);
				m_pivot[1] = args.getInt("pivot_y", 0);
			}
			else if (section == "animation")
			{
				Anim anim;
				anim.name = args.getString("name", "(noname)");
				const int gridX = args.getInt("grid_x", 0);
				const int gridY = args.getInt("grid_y", 0);
				anim.firstCell = gridX + gridY * m_animCellCount[0];
				anim.numFrames = args.getInt("frames", 1);
				anim.frameRate = args.getInt("rate", 1);
				anim.pivot[0] = args.getInt("pivot_x", m_pivot[0]);
				anim.pivot[1] = args.getInt("pivot_y", m_pivot[1]);
				anim.loop = args.getInt("loop", 1) != 0;
				anim.frameTriggers.resize(anim.numFrames);
				currentAnim = &m_animMap.insert(AnimMap::value_type(anim.name, anim)).first->second;
			}
			else if (section == "trigger")
			{
				if (currentAnim == 0)
				{
					logError("cannot add trigger when no animation defined yet");
				}
				else
				{
					const int frame = args.getInt("frame", 0);
					const std::string event = args.getString("on", "enter");
					const std::string action = args.getString("action", "");
					
					if (frame >= currentAnim->numFrames)
					{
						logError("value for 'frame' is out of range: %s", line.c_str());
					}
					else
					{
						bool ok = true;
						
						AnimTrigger::Event eventEnum;
						
						if (event == "enter")
							eventEnum = AnimTrigger::OnEnter;
						else if (event == "leave")
							eventEnum = AnimTrigger::OnLeave;
						else
						{
							logError("invalid value for 'on", line.c_str());
							ok = false;
						}
						
						if (ok)
						{
							//log("added frame trigger. frame=%d, on=%s, action=%s", frame, event.c_str(), action.c_str());
							
							AnimTrigger trigger;
							trigger.event = eventEnum;
							trigger.action = action;
							trigger.args = args;
							
							currentAnim->frameTriggers[frame].push_back(trigger);
						}
					}
				}
			}
			else
			{
				logError("unknown section: %s (%s)", line.c_str(), section.c_str());
			}
		}
	}
}

int AnimCacheElem::getVersion() const
{
	return g_globals.g_resourceVersion;
}

void AnimCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void AnimCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

AnimCacheElem & AnimCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		AnimCacheElem elem;
		
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
		g_soundPlayer.checkError();
		buffer = 0;
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
		
		//printf("added glyph cache element. face=%p, size=%d, character=%c, texture=%u. count=%d\n", face, size, c, elem.texture, (int)m_map.size());
		
		return i->second;
	}
}
