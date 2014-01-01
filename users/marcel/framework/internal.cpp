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
	textures = 0;
	sx = sy = 0;
	gridSx = gridSy = 0;
}

void TextureCacheElem::free()
{
	if (textures != 0)
	{
		const int numTextures = gridSx * gridSy;
		glDeleteTextures(numTextures, textures);
		delete [] textures;
		
		name.clear();
		textures = 0;
		sx = sy = 0;
		gridSx = gridSy = 0;
	}
}

void TextureCacheElem::load(const char * filename, int gridSx, int gridSy)
{
	free();
	
	name = filename;
	
	ImageData * imageData = loadImage(filename);
	
	if (!imageData)
	{
		logError("failed to load %s", filename);
	}
	else
	{
		if ((imageData->sx % gridSx) != 0 || (imageData->sy % gridSy) != 0)
		{
			logError("image size (%d, %d) must be a multiple of the grid size (%d, %d)",
				imageData->sx, imageData->sy, gridSx, gridSy);
		}
		else
		{
			const int numTextures = gridSx * gridSy;
			const int cellSx = imageData->sx / gridSx;
			const int cellSy = imageData->sy / gridSy;
			
			textures = new GLuint[numTextures];
			
			glGenTextures(numTextures, textures);
			
			for (int i = 0; i < numTextures; ++i)
			{
				fassert(textures[i] != 0);
				
				if (textures[i] != 0)
				{
					const int cellX = i % gridSx;
					const int cellY = i / gridSx;
					const int sourceX = cellX * cellSx;
					const int sourceY = cellY * cellSy;
					const int sourceOffset = sourceX + sourceY * imageData->sx;
					
					// capture current OpenGL states before we change them
					
					GLuint restoreTexture;
					glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
					GLint restoreUnpackAlignment;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpackAlignment);
					GLint restoreUnpackRowLength;
					glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restoreUnpackRowLength);
					
					// copy image data
					
					const void * source = ((int*)imageData->imageData) + sourceOffset;
					
					glBindTexture(GL_TEXTURE_2D, textures[i]);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, imageData->sx);
					glTexImage2D(
						GL_TEXTURE_2D,
						0,
						GL_RGBA,
						cellSx,
						cellSy,
						0,
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						source);
					
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
					glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpackAlignment);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, restoreUnpackRowLength);
				}
				
				this->sx = imageData->sx;
				this->sy = imageData->sy;
				this->gridSx = gridSx;
				this->gridSy = gridSy;
			}
			
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
		i->second.load(i->first.name.c_str(), i->first.gridSx, i->first.gridSy);
	}
}

TextureCacheElem & TextureCache::findOrCreate(const char * name, int gridSx, int gridSy)
{
	Key key;
	key.name = name;
	key.gridSx = gridSx;
	key.gridSy = gridSy;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		TextureCacheElem elem;
		
		elem.load(name, gridSx, gridSy);
		
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
	m_gridSize[0] = m_gridSize[1] = 1;
	m_pivot[0] = m_pivot[1] = 0;
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
	
	if (!r.open(filename, true))
	{
		//logError("%s: failed to open file!", filename);
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
				logError("%s: missing parameters: %s (%s)", filename, line.c_str(), parts[0].c_str());
				continue;
			}
			
			const std::string section = parts[0];
			Dictionary args;
			
			for (size_t i = 1; i < parts.size(); ++i)
			{
				const size_t separator = parts[i].find(':');
				
				if (separator == std::string::npos)
				{
					logError("%s: incorrect key:value syntax: %s (%s)", filename, line.c_str(), parts[i].c_str());
					continue;
				}
				
				const std::string key = parts[i].substr(0, separator);
				const std::string value = parts[i].substr(separator + 1, parts[i].size() - separator - 1);
				
				if (key.size() == 0 || value.size() == 0)
				{
					logError("%s: incorrect key:value syntax: %s (%s)", filename, line.c_str(), parts[i].c_str());
					continue;
				}
				
				if (args.contains(key.c_str()))
				{
					logError("%s: duplicate key: %s (%s)", filename, line.c_str(), key.c_str());
					continue;
				}
				
				args.setString(key.c_str(), value.c_str());
			}
			
			// sheet pivot_x:0 pivot_y:0 cells_x:4 cells_y:8
			// animation name:walk grid_x:0 grid_y:0 frames:12 rate:4 loop:0 pivot_x:2 pivot_y:2
			// trigger frame:3 action:sound sound:test.wav
			
			if (section == "sheet")
			{
				const int gridSx = args.getInt("grid_sx", 1);
				const int gridSy = args.getInt("grid_sy", 1);
				if (gridSx <= 0 || gridSy <= 0)
				{
					logError("%s: grid size must be > 0: %s", filename, line.c_str());
					continue;
				}
				m_gridSize[0] = gridSx;
				m_gridSize[1] = gridSy;
				m_pivot[0] = args.getInt("pivot_x", 0);
				m_pivot[1] = args.getInt("pivot_y", 0);
			}
			else if (section == "animation")
			{
				currentAnim = 0;
				
				Anim anim;
				anim.name = args.getString("name", "");
				if (anim.name.empty())
				{
					logError("%s: name not set: %s", filename, line.c_str());
					continue;
				}
				const int gridX = args.getInt("grid_x", 0);
				const int gridY = args.getInt("grid_y", 0);
				if (gridX < 0 || gridY < 0)
				{
					logError("%s: grid_x and grid_y must be >= 0: %s", filename, line.c_str());
					continue;
				}
				anim.firstCell = gridX + gridY * m_gridSize[0];
				anim.numFrames = args.getInt("frames", 1);
				const int lastCell = anim.firstCell + anim.numFrames;
				if (lastCell > m_gridSize[0] * m_gridSize[1])
				{
					logError("%s: animation lies (partially or completely) outside the grid: %s", filename, line.c_str());
					continue;
				}
				anim.frameRate = (float)args.getInt("rate", 1);
				if (anim.frameRate <= 0)
				{
					logError("%s: frame rate must be >= 1: %s", filename, line.c_str());
					continue;
				}
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
					logError("%s: must first define an animation before adding triggers to it! %s", filename, line.c_str());
					continue;
				}
				
				const int frame = args.getInt("frame", 0);
				
				if (frame < 0 || frame >= currentAnim->numFrames)
				{
					logWarning("%s: frame is not a key frame within the animation: %s", filename, line.c_str());
					continue;
				}
				
				const std::string event = args.getString("on", "enter");
				const std::string action = args.getString("action", "");
								
				AnimTrigger::Event eventEnum;
				
				if (event == "enter")
					eventEnum = AnimTrigger::OnEnter;
				else if (event == "leave")
					eventEnum = AnimTrigger::OnLeave;
				else
				{
					logError("%s: invalid value for 'on': %s", filename, line.c_str());
					continue;
				}
				
				//log("added frame trigger. frame=%d, on=%s, action=%s", frame, event.c_str(), action.c_str());
				
				AnimTrigger trigger;
				trigger.event = eventEnum;
				trigger.action = action;
				trigger.args = args;
				
				currentAnim->frameTriggers[frame].push_back(trigger);
			}
			else
			{
				logError("%s: unknown section: %s (%s)", filename, line.c_str(), section.c_str());
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
		ALenum bufferFormat = (ALenum)-1;
		
		if (soundData->channelCount == 1)
		{
			if (soundData->channelSize == 1)
				bufferFormat = AL_FORMAT_MONO8;
			else if (soundData->channelSize == 2)
				bufferFormat = AL_FORMAT_MONO16;
		}
		else if (soundData->channelCount == 2)
		{
			if (soundData->channelSize == 1)
				bufferFormat = AL_FORMAT_STEREO8;
			else if (soundData->channelSize == 2)
				bufferFormat = AL_FORMAT_STEREO16;
		}
		
		if (bufferFormat != -1)
		{
			alGenBuffers(1, &buffer);
			g_soundPlayer.checkError();
			
			if (buffer != 0)
			{
				ALuint bufferSize = soundData->sampleCount * soundData->channelCount * soundData->channelSize;
				ALuint bufferSampleRate = soundData->sampleRate;
				
				log("%s: buffer=%u, bufferFormat=%d, samleData=%p, bufferSize=%d, sampleRate=%d",
					filename,
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
				logError("%s: failed to create OpenAL buffer", filename);
			}
		}
		else
		{
			logError("%s: unknown channel config: channelCount=%d, channelSize=%d",
				filename,
				soundData->channelCount,
				soundData->channelSize);
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
		logError("%s: unable to open font", filename);
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
		
		//log("added glyph cache element. face=%p, size=%d, character=%c, texture=%u. count=%d\n", face, size, c, elem.texture, (int)m_map.size());
		
		return i->second;
	}
}
