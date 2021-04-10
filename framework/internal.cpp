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

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#elif defined(ANDROID)
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

#include "audio.h"
#include "image.h"
#include "internal.h"
#include "spriter.h"
#include "StringEx.h"

#if USE_GLYPH_ATLAS
	#include "textureatlas.h"
#endif

#if ENABLE_MSDF_FONTS
	#include "msdfgen.h"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// for resource caching
#include "FileStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

#if defined(DEBUG)
	#include "Timer.h"
#endif

Globals globals;

TextureCache g_textureCache;
Texture3dCache g_texture3dCache;
AnimCache g_animCache;
SpriterCache g_spriterCache;
SoundCache g_soundCache;
FontCache g_fontCache;
#if ENABLE_MSDF_FONTS
MsdfFontCache g_fontCacheMSDF;
#endif
GlyphCache g_glyphCache;

std::vector<ShaderOutput> g_shaderOutputs;

// -----

void checkErrorGL_internal(const char * function, int line)
{
#if FRAMEWORK_ENABLE_GL_ERROR_LOG
	const GLenum error = glGetError();
	
	if (error != GL_NO_ERROR)
	{
		logError("%s: %d: OpenGL error: %x", function, line, error);
		AssertMsg(error == GL_NO_ERROR, "%s: %d: OpenGL error: %x", function, line, error);
	}
#endif
}

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT && ENABLE_DESKTOP_OPENGL
static void formatDebugOutputGL(char * outStr, size_t outStrSize, GLenum source, GLenum type, GLuint id, GLenum severity, const char * msg)
{
	char sourceStr[32];
	const char *sourceFmt = "UNDEFINED(0x%04X)";
	switch(source)
	{
	case GL_DEBUG_SOURCE_API_ARB:             sourceFmt = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   sourceFmt = "WINDOW_SYSTEM"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: sourceFmt = "SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     sourceFmt = "THIRD_PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB:     sourceFmt = "APPLICATION"; break;
	case GL_DEBUG_SOURCE_OTHER_ARB:           sourceFmt = "OTHER"; break;
	}
	sprintf_s(sourceStr, 32, sourceFmt, source);

	char typeStr[32];
	const char *typeFmt = "UNDEFINED(0x%04X)";
	switch(type)
	{
	case GL_DEBUG_TYPE_ERROR_ARB:               typeFmt = "ERROR"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: typeFmt = "DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  typeFmt = "UNDEFINED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB:         typeFmt = "PORTABILITY"; break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB:         typeFmt = "PERFORMANCE"; break;
	case GL_DEBUG_TYPE_OTHER_ARB:               typeFmt = "OTHER"; break;
	}
	sprintf_s(typeStr, 32, typeFmt, type);

	char severityStr[32];
	const char *severityFmt = "UNDEFINED";
	switch(severity)
	{
	case GL_DEBUG_SEVERITY_HIGH_ARB:   severityFmt = "HIGH";   break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB: severityFmt = "MEDIUM"; break;
	case GL_DEBUG_SEVERITY_LOW_ARB:    severityFmt = "LOW"; break;
	}
	sprintf_s(severityStr, 32, severityFmt, severity);

	sprintf_s(outStr, outStrSize, "OpenGL: %s [source=%s type=%s severity=%s id=%d]", msg, sourceStr, typeStr, severityStr, id);
}

#if defined(WIN32)
void __stdcall debugOutputGL(
#else
void debugOutputGL(
#endif
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar * message,
	const GLvoid * userParam)
{
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		FILE * file = (FILE*)userParam;
		char formattedMessage[4096];
		formatDebugOutputGL(formattedMessage, sizeof(formattedMessage), source, type, id, severity, message);
		fprintf(file, "%s\n", formattedMessage);
	}
}
#endif

//

#if defined(DEBUG)

ScopedLoadTimer::ScopedLoadTimer(const char * filename)
	: m_filename(filename)
{
	logDebug("load %s [begin]", m_filename);

	m_startTime = g_TimerRT.TimeUS_get();
}

ScopedLoadTimer::~ScopedLoadTimer()
{
	const uint64_t endTime = g_TimerRT.TimeUS_get();

	logDebug("load %s [end] took %02.2fms", m_filename, (endTime - m_startTime) / 1000.f);
}

#endif

// -----

#if USE_STBFONT || ENABLE_MSDF_FONTS

StbFont::StbFont()
	: buffer(nullptr)
	, bufferSize(0)
{
}

StbFont::~StbFont()
{
	free();
}

bool StbFont::load(const char * filename)
{
	bool result = false;
	
	FILE * file = fopen(filename, "rb");
	
	if (file != nullptr)
	{
		// load source from file

		Verify(fseek(file, 0, SEEK_END) == 0);
		{
			const int64_t fileSize = ftell(file);
			Assert(fileSize >= 0);
			bufferSize = (size_t)fileSize;
		}
		Verify(fseek(file, 0, SEEK_SET) == 0);

		buffer = new uint8_t[bufferSize];

		if (fread(buffer, 1, bufferSize, file) == (size_t)bufferSize)
		{
			if (stbtt_InitFont(&fontInfo, buffer, 0) != 0)
			{
				result = true;
			}
		}
		
		fclose(file);
		file = nullptr;
	}
	
	if (result == false)
	{
		free();
	}
	
	return result;
}

void StbFont::free()
{
	delete[] buffer;
	buffer = nullptr;
	
	bufferSize = 0;
}

#endif

// -----
	
TextureCacheElem::TextureCacheElem()
{
	textures = 0;
	sx = sy = 0;
	gridSx = gridSy = 0;
	mipmapped = false;
}

void TextureCacheElem::free()
{
	if (textures != 0)
	{
		const int numTextures = gridSx * gridSy;
		for (int i = 0; i < numTextures; ++i)
			textures[i].free();
		delete [] textures;
		
		name.clear();
		textures = 0;
		sx = sy = 0;
		gridSx = gridSy = 0;
		mipmapped = false;
	}
}

#if defined(WINDOWS)
#include <Windows.h>
#include <Pathcch.h>
static std::string getCacheFilename(const char * filename, bool forRead)
{
	const int kPathSize = 256;
	char tempPath[kPathSize];

	if (GetTempPathA(kPathSize, tempPath) > 0)
	{
		uint32_t hash = 0;
		for (int i = 0; filename[i]; ++i)
			hash = hash * 13 + filename[i];
		char hashName[32];
		sprintf_s(hashName, sizeof(hashName), "fwc-%08x.bin", hash);
		strcat_s(tempPath, sizeof(tempPath), hashName);

		if (forRead)
		{
			// check timestamp on both files
			bool outdated = false;
			const HANDLE file1 = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			const HANDLE file2 = CreateFile(tempPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file1 == INVALID_HANDLE_VALUE || file2 == INVALID_HANDLE_VALUE)
				outdated = true;
			else
			{
				FILETIME fileTime1;
				FILETIME fileTime2;
				if (!GetFileTime(file1, NULL, NULL, &fileTime1) ||
					!GetFileTime(file2, NULL, NULL, &fileTime2) ||
					CompareFileTime(&fileTime1, &fileTime2) == 1)
					outdated = true;
			}
			if (file1 != INVALID_HANDLE_VALUE)
				CloseHandle(file1);
			if (file2 != INVALID_HANDLE_VALUE)
				CloseHandle(file2);
			if (outdated)
				return "";
		}

		return tempPath;
	}

	return "";
}
#elif defined(MACOS)
#include <sys/stat.h>
#include <unistd.h>
static std::string getCacheFilename(const char * filename, bool forRead)
{
	char path[PATH_MAX];
	size_t n = confstr(_CS_DARWIN_USER_CACHE_DIR, path, sizeof(path));
	if (n == 0)
		return "";
	else
	{
		if (n >= 2 && path[n - 2] == '/')
			path[n - 2] = 0;
		
		uint32_t hash = 0;
		for (int i = 0; filename[i]; ++i)
			hash = hash * 13 + filename[i];
		
		char tempPath[PATH_MAX];
		sprintf_s(tempPath, sizeof(tempPath), "%s/fwc-%08x.bin", path, hash);
		
		if (forRead)
		{
			// check timestamp on both files
			bool outdated = false;
			FILE * file1 = fopen(filename, "rb");
			FILE * file2 = fopen(tempPath, "rb");
			if (file1 == nullptr || file2 == nullptr)
				outdated = true;
			else
			{
				struct stat stat1;
				struct stat stat2;
				if (fstat(fileno(file1), &stat1) != 0 || fstat(fileno(file2), &stat2) != 0)
					outdated = true;
				else if (stat1.st_mtime > stat2.st_mtime)
					outdated = true;
			}
			if (file1 != nullptr)
				fclose(file1);
			if (file2 != nullptr)
				fclose(file2);
			if (outdated)
				return "";
		}
		
		return tempPath;
	}
}
#else
static std::string getCacheFilename(const char * filename, bool forRead)
{
	return "";
}
#endif

void TextureCacheElem::load(const char * filename, int gridSx, int gridSy, bool mipmapped)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	name = filename;
	
	ImageData * imageData = 0;

	if (framework.cacheResourceData)
	{
		const std::string cacheFilename = getCacheFilename(filename, true);

		if (!cacheFilename.empty() && FileStream::Exists(cacheFilename.c_str()))
		{
			try
			{
				FileStream stream(cacheFilename.c_str(), OpenMode_Read);
				StreamReader reader(&stream, false);

				const uint32_t version = reader.ReadUInt32();

				if (version == 0)
				{
					const uint32_t sx = reader.ReadUInt32();
					const uint32_t sy = reader.ReadUInt32();

					imageData = new ImageData(sx, sy);

					reader.ReadBytes(imageData->imageData, imageData->sx * imageData->sy * sizeof(ImageData::Pixel));
				}
			}
			catch (std::exception & e)
			{
				logError("failed to read cache data: %s", e.what());
				(void)e;

				delete imageData;
				imageData = 0;
			}
		}
	}

	if (!imageData)
	{
		imageData = loadImage(filename);

		if (framework.cacheResourceData && imageData)
		{
			const std::string cacheFilename = getCacheFilename(filename, false);
			
			if (cacheFilename.empty())
			{
				logWarning("failed to get cache filename for: %s", filename);
			}
			else
			{
				try
				{
					FileStream stream(cacheFilename.c_str(), OpenMode_Write);
					StreamWriter writer(&stream, false);

					writer.WriteUInt32(0); // version number
					writer.WriteUInt32(imageData->sx);
					writer.WriteUInt32(imageData->sy);

					writer.WriteBytes(imageData->imageData, imageData->sx * imageData->sy * sizeof(ImageData::Pixel));
				}
				catch (std::exception & e)
				{
					logError("failed to write cache data: %s", e.what());
					(void)e;
				}
			}
		}
	}
	
	if (!imageData)
	{
		logError("failed to load %s (%dx%d)", filename, gridSx, gridSy);
	}
	else
	{
	#if 1 // imageFixAlphaFilter for png and gif
		if (String::EndsWith(filename, ".png") || String::EndsWith(filename, ".gif"))
		{
			ImageData * temp = imageFixAlphaFilter(imageData);
			delete imageData;
			imageData = temp;
		}
	#endif

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
			
			textures = new GxTexture[numTextures];
			
			GxTextureProperties textureProperties;
			textureProperties.dimensions.sx = cellSx;
			textureProperties.dimensions.sy = cellSy;
			textureProperties.format = GX_RGBA8_UNORM;
			textureProperties.sampling.filter = false;
			textureProperties.sampling.clamp = true;
			textureProperties.mipmapped = mipmapped;
			
			// todo : try to see if there's a meta data file for this texture,
			//        if so, load texture settings from this file (similar to Sprite)
			//     _OR_ when getTexture is called on a .txt file, load the .txt
			//        file and reference the texture from the .txt file or by replacing the extension <-- this doesn't seem like a good idea. we want people to be able to override texture settings (for instance through refine) without the code needing to change
			//    idea : for refine : make sure most resources have reflection data. load/save of meta data files would be much easier this way. also, enables an 'advanced' section inside the resource editor, which lets one inspect and modify the entire reflected structure. eg, the sprite editor would have a nice UI for changing the scale & flipped flags, and an advanced section for setting grid divisions, animation definitions
			//    -> use meta data to see if mipmaps should be enabled, and for filter settings
			
			for (int i = 0; i < numTextures; ++i)
			{
				const int cellX = i % gridSx;
				const int cellY = i / gridSx;
				const int sourceX = cellX * cellSx;
				const int sourceY = cellY * cellSy;
				const int sourceOffset = sourceX + sourceY * imageData->sx;
				
				const void * source = ((int*)imageData->imageData) + sourceOffset;
				
				textures[i].allocate(textureProperties);
				textures[i].upload(source, 0, imageData->sx, true);
			}
			
			this->sx = imageData->sx;
			this->sy = imageData->sy;
			this->gridSx = gridSx;
			this->gridSy = gridSy;
			this->mipmapped = mipmapped;
			
			logInfo("loaded %s (%dx%d)", filename, gridSx, gridSy);
		}
		
		delete imageData;
	}
}

void TextureCacheElem::reload()
{
	const std::string oldName = name;
	const int oldGridSx = gridSx;
	const int oldGridSy = gridSy;
	const bool oldMipmapped = mipmapped;

	free();

	load(oldName.c_str(), oldGridSx, oldGridSy, oldMipmapped);
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
		i->second.reload();
	}
}

TextureCacheElem & TextureCache::findOrCreate(const char * name, int gridSx, int gridSy, bool mipmapped)
{
	Key key;
	key.name = name;
	key.gridSx = gridSx;
	key.gridSy = gridSy;
	key.mipmapped = mipmapped;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		TextureCacheElem elem;
		
		elem.load(resolved_filename, gridSx, gridSy, mipmapped);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

Texture3dCacheElem::Texture3dCacheElem()
{
	texture = nullptr;
}

void Texture3dCacheElem::free()
{
	if (texture != nullptr)
	{
		texture->free();
		
		delete texture;
		texture = nullptr;
	}
}

void Texture3dCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	name = filename;
	texture = new GxTexture3d();
	
	ImageData * imageData = loadImage(filename);

	if (!imageData)
	{
		logError("failed to load %s", filename);
	}
	else
	{
	#if 1 // imageFixAlphaFilter for png and gif
		if (String::EndsWith(filename, ".png") || String::EndsWith(filename, ".gif"))
		{
			ImageData * temp = imageFixAlphaFilter(imageData);
			delete imageData;
			imageData = temp;
		}
	#endif
		
		GxTexture3dProperties textureProperties;
		if (imageData->sx > 0 && imageData->sy > 0)
		{
			textureProperties.dimensions.sx = imageData->sx / imageData->sy;
			textureProperties.dimensions.sy = imageData->sy;
			textureProperties.dimensions.sz = imageData->sx / imageData->sy;
		}
		textureProperties.format = GX_RGBA8_UNORM;
		textureProperties.mipmapped = false;
		
	// todo : try to see if there's a meta data file for this texture,
	//        if so, load texture settings from this file (similar to Sprite)
	//     _OR_ when getTexture3d is called on a .txt file, load the .txt
	//        file and reference the texture from the .txt file or by replacing the extension
	//    -> use meta data to see if mipmaps should be enabled

		texture->allocate(textureProperties);
		texture->upload(imageData->imageData, 4, 0);

		logInfo("loaded %s", filename);
	}
	
	delete imageData;
}

void Texture3dCacheElem::reload()
{
	const std::string oldName = name;

	free();

	load(oldName.c_str());
}

void Texture3dCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void Texture3dCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.reload();
	}
}

Texture3dCacheElem & Texture3dCache::findOrCreate(const char * name)
{
	Key key;
	key.name = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		Texture3dCacheElem elem;
		
		elem.load(resolved_filename);
		
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
	m_hasSheet = false;
	m_gridSize[0] = m_gridSize[1] = 1;
	m_pivot[0] = m_pivot[1] = 0;
	m_scale = 1.f;
	m_animMap.clear();
}

template <typename Policy>
void splitString(const std::string & str, std::vector<std::string> & result, Policy policy, const bool keepEmptyElements)
{
	if (str.empty())
		return;
		
	size_t start = -1;
	bool foundStart = false;
	
	for (size_t i = 0; i <= str.size(); ++i)
	{
		const char c = i < str.size() ? str[i] : policy.getBreakChar();
		
		if (foundStart == false)
		{
			// found start
			if (!policy.isBreak(c))
			{
				start = i;
				foundStart = true;
			}
			else if (keepEmptyElements)
			{
				result.push_back(std::string());
			}
		}
		else if (policy.isBreak(c))
		{
			// found end
			result.push_back(str.substr(start, i - start));
			
			start = -1;
			foundStart = false;
		}
	}
}

class WhiteSpacePolicy
{
public:
	bool isBreak(char c)
	{
		return c == '\t' || c == ' ' || c == '\r' || c == '\n';
	}
	char getBreakChar()
	{
		return ' ';
	}
};

void splitString(const std::string & str, std::vector<std::string> & result, bool keepEmptyElements)
{
	WhiteSpacePolicy policy;
	
	splitString<WhiteSpacePolicy>(str, result, policy, keepEmptyElements);
}

void splitString(const std::string & str, std::vector<std::string> & result)
{
	WhiteSpacePolicy policy;
	
	splitString<WhiteSpacePolicy>(str, result, policy, false);
}

class CharPolicy
{
	char C;
public:
	CharPolicy(char c)
	{
		C = c;
	}
	bool isBreak(char c)
	{
		return c == C;
	}
	char getBreakChar()
	{
		return C;
	}
};

void splitString(const std::string & str, std::vector<std::string> & result, char c)
{
	CharPolicy policy(c);
	
	splitString<CharPolicy>(str, result, policy, true);
}

void AnimCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

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
				m_hasSheet = true;
				m_gridSize[0] = gridSx;
				m_gridSize[1] = gridSy;
				m_pivot[0] = args.getInt("pivot_x", 0);
				m_pivot[1] = args.getInt("pivot_y", 0);
				m_scale = args.getFloat("scale", 1.f);
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
				anim.loopStart = args.getInt("loop_start", 0);
				if (anim.loopStart >= anim.numFrames)
				{
					logError("%s: loop start must be < 'frames': %s", filename, line.c_str());
					continue;
				}
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
				
				//logInfo("added frame trigger. frame=%d, on=%s, action=%s", frame, event.c_str(), action.c_str());
				
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
	return globals.resourceVersion;
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
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		AnimCacheElem elem;
		
		elem.load(resolved_filename);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

SpriterCacheElem::SpriterCacheElem()
	: m_scene(0)
{
}

void SpriterCacheElem::free()
{
	delete m_scene;
	m_scene = 0;
}

void SpriterCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	m_scene = new spriter::Scene();
	
	if (m_scene->load(filename))
	{
		logInfo("loaded spriter %s", filename);
	}
	else
	{
		logError("failed to load spriter scene: %s", filename);
	}
}

void SpriterCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void SpriterCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

SpriterCacheElem & SpriterCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		SpriterCacheElem elem;
		
		elem.load(resolved_filename);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

SoundCacheElem::SoundCacheElem()
{
	buffer = nullptr;
}

void SoundCacheElem::free()
{
#if FRAMEWORK_USE_SOUNDPLAYER_USING_AUDIOSTREAM || FRAMEWORK_USE_PORTAUDIO
	if (buffer != nullptr)
	{
		g_soundPlayer.stopSoundsForBuffer(buffer);
		
		g_soundPlayer.destroyBuffer(buffer);
	}
#endif
}

void SoundCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	SoundData * soundData = loadSound(filename);
	
	if (soundData != 0)
	{
	#if FRAMEWORK_USE_SOUNDPLAYER_USING_AUDIOSTREAM || FRAMEWORK_USE_PORTAUDIO
		buffer = g_soundPlayer.createBuffer(
			soundData->sampleData,
			soundData->sampleCount,
			soundData->sampleRate,
			soundData->channelSize,
			soundData->channelCount);

		if (buffer != nullptr)
		{
			logInfo("loaded %s", filename);
		}
		else
		{
			logError("%s: failed to create sound buffer", filename);
		}
	#endif
		
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
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		SoundCacheElem elem;
		
		elem.load(resolved_filename);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

FontCacheElem::FontCacheElem()
{
#if USE_STBFONT
	font = nullptr;
#elif USE_FREETYPE
	face = 0;
#endif

#if USE_GLYPH_ATLAS
	textureAtlas = nullptr;
#endif
}

void FontCacheElem::free()
{
#if USE_STBFONT
	if (font != nullptr)
	{
		font->free();
		
		delete font;
		font = nullptr;
	}
#elif USE_FREETYPE
	if (face != 0)
	{
		if (FT_Done_Face(face) != 0)
		{
			logError("failed to free face");
		}
		
		face = 0;
	}
#endif
	
#if USE_GLYPH_ATLAS
	if (textureAtlas != nullptr)
	{
		delete textureAtlas;
		textureAtlas = nullptr;
	}
#endif
}

void FontCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	//
	
	bool loaded = false;
	
#if USE_STBFONT
	font = new StbFont();
	
	if (font->load(filename))
	{
		loaded = true;
	}
#elif USE_FREETYPE
	if (FT_New_Face(globals.freeType, filename, 0, &face) != 0)
	{
		logError("%s: unable to open font", filename);
		face = 0;
	}
	else
	{
		logInfo("loaded %s", filename);
		
		const FT_Error err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		Assert(err == 0);
		if (err != 0)
			logWarning("failed to select FreeType unicode character map");
		
		loaded = true;
	}
#endif
	
#if USE_GLYPH_ATLAS
	if (loaded)
	{
		textureAtlas = new TextureAtlas();
		textureAtlas->init(256, 16, GX_R8_UNORM, false, false, nullptr);
	}
	else
	{
		textureAtlas = new TextureAtlas();
		textureAtlas->init(1, 1, GX_R8_UNORM, false, false, nullptr);
	}
#endif
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
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		FontCacheElem elem;
		
		elem.load(resolved_filename);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

// -----

void GlyphCache::clear()
{
#if USE_GLYPH_ATLAS
	// note : clearing the font cache will free the texture atlasses and the texture atlas elements with it
	//        there's no need to free them manually here
#else
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		if (i->second.texture != 0)
		{
			glDeleteTextures(1, &i->second.texture);
			checkErrorGL();
		}
	}
#endif
	
	m_map.clear();
}

#if USE_STBFONT

GlyphCacheElem & GlyphCache::findOrCreate(const StbFont * font, int size, int c)
{
	fassert(font == globals.font->font);
	
	Key key;
	key.font = font;
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
		
		const float scale = stbtt_ScaleForPixelHeight(&font->fontInfo, size);
		
		int x1;
		int y1;
		int x2;
		int y2;
		
		// note : STBTT packing routines contain oversampling and filtering support ..
		//        but there's no way to access this functionality officially bypassing
		//        the packing routines. copy the implementation ? or use STBTT packing
		//        and texture atlas management ? I would rather not .. as the current
		//        implementation allows the texture atlas to grow and 'intelligently'
		//        updates the texture. also STBTT's packing seems to be oriented towards
		//        packing entire unicode ranges at once, while we want to just generate
		//        glyphs as we go for now (and the foreseeable future)
		
		stbtt_GetCodepointBitmapBox(&font->fontInfo, c, scale, scale, &x1, &y1, &x2, &y2);
		
		const int sx = x2 - x1;
		const int sy = y2 - y1;
		
		uint8_t * values = (uint8_t*)alloca(sx * sy);
		
		stbtt_MakeCodepointBitmap(
			&font->fontInfo,
			values,
			sx,
			sy,
			sx, scale, scale, c);
		
		for (;;)
		{
			elem.textureAtlasElem = globals.font->textureAtlas->tryAlloc(values, sx, sy, GLYPH_ATLAS_BORDER);
			
			if (elem.textureAtlasElem != nullptr)
				break;
			
			// note : texture atlas re-allocation shouldn't happen in draw code; make sure to cache each glyph elem first before calling gxBegin!
				
			globals.font->textureAtlas->makeBiggerAndOptimize(globals.font->textureAtlas->a.sx, globals.font->textureAtlas->a.sy * 2);
		}
		
		elem.sx = sx;
		elem.sy = sy;
		elem.y = y1;
		
		int advance;
		int lsb;
		stbtt_GetCodepointHMetrics(&font->fontInfo, c, &advance, &lsb);

		elem.advance = advance;
		elem.lsb = lsb;
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		//logInfo("added glyph cache element. face=%p, size=%d, character=%c, texture=%u. count=%d\n", face, size, c, elem.texture, (int)m_map.size());
		
		return i->second;
	}
}

#elif USE_FREETYPE

GlyphCacheElem & GlyphCache::findOrCreate(FT_Face face, int size, int c)
{
// todo : add retina support to glyph cache. scale size by backing store scale, but draw at normal size
	fassert(face == globals.font->face);
	
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
		
		const FT_Error err = FT_Set_Pixel_Sizes(face, 0, size);
		(void)err; Assert(err == FT_Err_Ok);

		// note : we use FT_LOAD_NO_BITMAP to avoid getting embedded glyph data. this embedded data uses 1 bpp monochrome pre-rendered versions of the glyphs. we currently do not support unpacking this data, although it may be beneficial (readability-wise) to do so
		// note : we use FT_LOAD_FORCE_AUTOHINT to improve readability for some fonts at small sizes. perhaps this flag can be removed when FT_LOAD_NO_BITMAP is removed
		
		if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_NO_BITMAP | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL) == 0)
		{
		#if USE_GLYPH_ATLAS
			elem.g = *face->glyph;
			
			BoxAtlasElem * e = nullptr;
			
			for (;;)
			{
				e = globals.font->textureAtlas->tryAlloc(elem.g.bitmap.buffer, elem.g.bitmap.width, elem.g.bitmap.rows, GLYPH_ATLAS_BORDER);
				
				if (e != nullptr)
					break;
				
				logDebug("glyph allocation failed. growing texture atlas to twice the old height");
				
				// note : texture atlas re-allocation shouldn't happen in draw code; make sure to cache each glyph elem first before calling gxBegin!
				
				globals.font->textureAtlas->makeBiggerAndOptimize(globals.font->textureAtlas->a.sx, globals.font->textureAtlas->a.sy * 2);
			}
			
			elem.textureAtlasElem = e;
		#else
			// capture current OpenGL states before we change them
			
		#if !ENABLE_OPENGL
			#error "Non-glyph atlas font implementation is written against OpenGL 2.1."
		#endif
		
			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			GLint restoreUnpack;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
			checkErrorGL();
			
			// create texture and copy image data
			
			elem.g = *face->glyph;
			glGenTextures(1, &elem.texture);
			checkErrorGL();

		#if USE_LEGACY_OPENGL
			glBindTexture(GL_TEXTURE_2D, elem.texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			checkErrorGL();

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
			checkErrorGL();
		#else
			glBindTexture(GL_TEXTURE_2D, elem.texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			checkErrorGL();

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				elem.g.bitmap.width,
				elem.g.bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				elem.g.bitmap.buffer);
			checkErrorGL();

			GLint swizzleMask[4] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			checkErrorGL();
		#endif
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			checkErrorGL();
			
			// restore previous OpenGL states
			
			glBindTexture(GL_TEXTURE_2D, restoreTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
			checkErrorGL();
		#endif
		}
		else
		{
			// failed to render the glyph. return the NULL texture handle
			
			logError("failed to render glyph");
		#if USE_GLYPH_ATLAS
			elem.textureAtlasElem = nullptr;
		#else
			elem.texture = 0;
		#endif
		}
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		//logInfo("added glyph cache element. face=%p, size=%d, character=%c, texture=%u. count=%d\n", face, size, c, elem.texture, (int)m_map.size());
		
		return i->second;
	}
}

#endif

//

#if ENABLE_MSDF_FONTS

MsdfGlyphCacheElem::MsdfGlyphCacheElem()
	: textureAtlasElem(nullptr)
	, y(0)
	, sx(0)
	, sy(0)
	, advance(0)
	, lsb(0)
	, isInitialized(false)
{
}

MsdfGlyphCache::MsdfGlyphCache()
	: m_isLoaded(false)
	, m_font()
	, m_textureAtlas(nullptr)
	, m_map()
{
}

MsdfGlyphCache::~MsdfGlyphCache()
{
	free();
}

//

void MsdfGlyphCache::free()
{
	m_map.clear();
	
	delete m_textureAtlas;
	m_textureAtlas = nullptr;
	
	m_font.free();
	
	//
	
	m_isLoaded = false;
}

void MsdfGlyphCache::allocTextureAtlas()
{
	const int kAtlasSx = 512;
	const int kAtlasSy = 512;
	
	delete m_textureAtlas;
	m_textureAtlas = nullptr;
	
	m_textureAtlas = new TextureAtlas();
	m_textureAtlas->init(kAtlasSx, kAtlasSy, GX_RGBA32_FLOAT, true, true, nullptr);
	
	m_map.clear();
}

void MsdfGlyphCache::load(const char * filename)
{
	free();
	
	//
	
	if (m_font.load(filename))
	{
		const std::string cacheFilename = std::string(filename) + ".cache";
		
		if (loadCache(cacheFilename.c_str()))
		{
			// done
		}
		else
		{
			allocTextureAtlas();
		}
		
		m_isLoaded = true;
	}
	
	if (m_isLoaded == false)
	{
		free();
	}
}

const MsdfGlyphCacheElem & MsdfGlyphCache::findOrCreate(const int codepoint)
{
	MsdfGlyphCacheElem & glyph = m_map[codepoint];
	
	// glyph is new. make sure it gets initialized here
				
	if (glyph.isInitialized == false && m_isLoaded)
	{
		makeGlyph(codepoint, glyph);
	}
	
	return glyph;
}

bool MsdfGlyphCache::stbGlyphToMsdfShape(const int codePoint, msdfgen::Shape & shape)
{
	stbtt_vertex * vertices = nullptr;
	
	const int numVertices = stbtt_GetCodepointShape(&m_font.fontInfo, codePoint, &vertices);
	
	msdfgen::Contour * contour = nullptr;
	
	msdfgen::Point2 old_p;
	
	for (int i = 0; i < numVertices; ++i)
	{
		stbtt_vertex & v = vertices[i];
		
		if (v.type == STBTT_vmove)
		{
			//logDebug("moveTo: %d, %d", v.x, v.y);
			
			contour = &shape.addContour();
			
			old_p.set(v.x, v.y);
			old_p *= MSDF_SCALE;
			
		}
		else if (v.type == STBTT_vline)
		{
			//logDebug("lineTo: %d, %d", v.x, v.y);
			
			msdfgen::Point2 new_p(v.x, v.y);
			new_p *= MSDF_SCALE;
			
			contour->addEdge(new msdfgen::LinearSegment(old_p, new_p));
			
			old_p = new_p;
		}
		else if (v.type == STBTT_vcurve)
		{
			//logDebug("quadraticTo: %d, %d", v.x, v.y);
			
			msdfgen::Point2 new_p(v.x, v.y);
			msdfgen::Point2 control_p(v.cx, v.cy);
			new_p *= MSDF_SCALE;
			control_p *= MSDF_SCALE;
			
			contour->addEdge(new msdfgen::QuadraticSegment(old_p, control_p, new_p));
			
			old_p = new_p;
		}
		else if (v.type == STBTT_vcubic)
		{
			//logDebug("cubicTo: %d, %d", v.x, v.y);
			
			msdfgen::Point2 new_p(v.x, v.y);
			msdfgen::Point2 control_p1(v.cx, v.cy);
			msdfgen::Point2 control_p2(v.cx1, v.cy1);
			new_p *= MSDF_SCALE;
			control_p1 *= MSDF_SCALE;
			control_p2 *= MSDF_SCALE;
			
			contour->addEdge(new msdfgen::CubicSegment(old_p, control_p1, control_p2, new_p));
			
			old_p = new_p;
		}
		else
		{
			logDebug("unknown vertex type: %d", v.type);
		}
	}
	
	stbtt_FreeShape(&m_font.fontInfo, vertices);
	vertices = nullptr;
	
	return true;
}

void MsdfGlyphCache::makeGlyph(const int codepoint, MsdfGlyphCacheElem & glyph)
{
	msdfgen::Shape shape;
	
	stbGlyphToMsdfShape(codepoint, shape);
	
	int x1, y1;
	int x2, y2;
	if (stbtt_GetCodepointBox(&m_font.fontInfo, codepoint, &x1, &y1, &x2, &y2) != 0)
	{
		//logDebug("glyph box: (%d, %d) - (%d, %d)", x1, y1, x2, y2);
		
		if (x2 < x1)
			std::swap(x1, x2);
		if (y2 < y1)
			std::swap(y1, y2);
		
		const int sx = x2 - x1 + 1;
		const int sy = y2 - y1 + 1;
		
		const float scaled_x1 = x1 * MSDF_SCALE;
		const float scaled_y1 = y1 * MSDF_SCALE;
		const float scaled_sx = sx * MSDF_SCALE;
		const float scaled_sy = sy * MSDF_SCALE;
		
		const int bitmapSx = (int)std::ceil(scaled_sx) + MSDF_GLYPH_PADDING_OUTER * 2;
		const int bitmapSy = (int)std::ceil(scaled_sy) + MSDF_GLYPH_PADDING_OUTER * 2;
		logDebug("msdf bitmap size: %d x %d", bitmapSx, bitmapSy);
		
		msdfgen::edgeColoringSimple(shape, 3.f);
		msdfgen::Bitmap<msdfgen::FloatRGBA> msdf(bitmapSx, bitmapSy);
		
		const msdfgen::Vector2 scaleVec(1.f, 1.f);
		const msdfgen::Vector2 transVec(
			-scaled_x1 + MSDF_GLYPH_PADDING_OUTER,
			-scaled_y1 + MSDF_GLYPH_PADDING_OUTER);
		msdfgen::generateMSDF(msdf, shape, 1.f, scaleVec, transVec);
		
		glyph.sx = sx;
		glyph.sy = sy;
		
		glyph.textureToGlyphScale[0] = sx / scaled_sx;
		glyph.textureToGlyphScale[1] = sy / scaled_sy;
		
		for (;;)
		{
			glyph.textureAtlasElem = m_textureAtlas->tryAlloc((uint8_t*)&msdf(0, 0), bitmapSx, bitmapSy);
			
			if (glyph.textureAtlasElem != nullptr)
				break;
			
			logDebug("MSDF glyph allocation failed. growing texture atlas to twice the old height");
				
			// note : texture atlas re-allocation shouldn't happen in draw code; make sure to cache each glyph elem first before calling gxBegin!
			
			m_textureAtlas->makeBiggerAndOptimize(m_textureAtlas->a.sx, m_textureAtlas->a.sy * 2);
		}
	}
	
	int advance;
	int lsb;
	stbtt_GetCodepointHMetrics(&m_font.fontInfo, codepoint, &advance, &lsb);
	
	glyph.isInitialized = true;
	glyph.advance = advance;
	glyph.lsb = lsb;
	glyph.y = y1;
}

bool MsdfGlyphCache::loadCache(const char * filename)
{
	bool result = true;
	
	FILE * file = nullptr;
	
	if (result == true)
	{
		file = fopen(filename, "rb");
		
		if (file == nullptr)
		{
			logDebug("loadCache: failed to open file");
			result = false;
		}
	}
	
	if (result == false)
	{
		return result;
	}
	
	//
	
	if (result == true)
	{
		int32_t version;
		
		result &= fread(&version, 4, 1, file) == 1;
		
		if (version != 4)
		{
			logDebug("loadCache: version mismatch");
			result = false;
		}
	}
	
	allocTextureAtlas();
	
	if (result == true)
	{
		// load glyphs
		
		int32_t numGlyphs = 0;
		
		result &= fread(&numGlyphs, 4, 1, file) == 1;
		
		for (int i = 0; result && i < numGlyphs; ++i)
		{
			int32_t c = 0;
			
			result &= fread(&c, 4, 1, file) == 1;
			
			//
			
			int32_t y;
			int32_t sx;
			int32_t sy;
			float textureToGlyphScale[2];
			int32_t advance;
			
			result &= fread(&y, 4, 1, file) == 1;
			result &= fread(&sx, 4, 1, file) == 1;
			result &= fread(&sy, 4, 1, file) == 1;
			result &= fread(&textureToGlyphScale[0], 4, 1, file) == 1;
			result &= fread(&textureToGlyphScale[1], 4, 1, file) == 1;
			result &= fread(&advance, 4, 1, file) == 1;
			
			//
			
			int32_t atlasElemSx;
			int32_t atlasElemSy;
			
			result &= fread(&atlasElemSx, 4, 1, file) == 1;
			result &= fread(&atlasElemSy, 4, 1, file) == 1;
			
			//
			
			if (result == false)
			{
				logDebug("loadCache: failed to load glyph info");
			}
			
			//
			
			BoxAtlasElem * ae = nullptr;
			
			if (atlasElemSx > 0 && atlasElemSy > 0)
			{
				const int numBytes = atlasElemSx * atlasElemSy * sizeof(float) * 4;
				uint8_t * bytes = new uint8_t[numBytes];
				
				result &= fread(bytes, numBytes, 1, file) == 1;
				
				if (result)
				{
					ae = m_textureAtlas->tryAlloc(bytes, atlasElemSx, atlasElemSy);
					
					result &= ae != nullptr;
				}
				
				delete[] bytes;
				
				if (result == false)
				{
					logDebug("loadCache: failed to load glyph data");
				}
			}
			
			if (result == true)
			{
				auto & glyph = m_map[c];
						
				glyph.textureAtlasElem = ae;
				glyph.y = y;
				glyph.sx = sx;
				glyph.sy = sy;
				glyph.textureToGlyphScale[0] = textureToGlyphScale[0];
				glyph.textureToGlyphScale[1] = textureToGlyphScale[1];
				glyph.advance = advance;
				glyph.isInitialized = true;
			}
		}
	}
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
	
	if (result == false)
	{
		logDebug("loadCache: failed to load cache");
		
		allocTextureAtlas();
	}
	
	return result;
}

bool MsdfGlyphCache::saveCache(const char * filename) const
{
	bool result = true;
	
	if (m_isLoaded == false)
	{
		return false;
	}
	
	FILE * file = nullptr;
	
	if (result == true)
	{
		file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			logDebug("saveCache: failed to open file");
			result = false;
		}
	}
	
	if (result == false)
	{
		return result;
	}
	
	//
	
	if (result == true)
	{
		const int32_t version = 4;
		
		result &= fwrite(&version, 4, 1, file) == 1;
	}
	
	if (result == true)
	{
		// save glyphs
		
		const int32_t numGlyphs = (int32_t)m_map.size();
		
		result &= fwrite(&numGlyphs, 4, 1, file) == 1;
		
		for (auto & i : m_map)
		{
			const int32_t c = i.first;
			
			result &= fwrite(&c, 4, 1, file) == 1;
			
			//
			
			const MsdfGlyphCacheElem & e = i.second;
			
			const int32_t y = e.y;
			const int32_t sx = e.sx;
			const int32_t sy = e.sy;
			const float textureToGlyphScale[2] =
			{
				e.textureToGlyphScale[0],
				e.textureToGlyphScale[1]
			};
			const int32_t advance = e.advance;
			
			result &= fwrite(&y, 4, 1, file) == 1;
			result &= fwrite(&sx, 4, 1, file) == 1;
			result &= fwrite(&sy, 4, 1, file) == 1;
			result &= fwrite(&textureToGlyphScale[0], 4, 1, file) == 1;
			result &= fwrite(&textureToGlyphScale[1], 4, 1, file) == 1;
			result &= fwrite(&advance, 4, 1, file) == 1;
			
			//
			
			const BoxAtlasElem * ae = e.textureAtlasElem;
			
			const int32_t atlasElemSx = ae ? ae->sx : 0;
			const int32_t atlasElemSy = ae ? ae->sy : 0;
			
			result &= fwrite(&atlasElemSx, 4, 1, file) == 1;
			result &= fwrite(&atlasElemSy, 4, 1, file) == 1;
			
			//
			
			if (result == false)
			{
				logDebug("saveCache: failed to save glyph info");
			}
			
			//
			
			if (atlasElemSx > 0 && atlasElemSy > 0)
			{
				const int numBytes = atlasElemSx * atlasElemSy * sizeof(float) * 4;
				uint8_t * bytes = new uint8_t[numBytes];
				
				result &= m_textureAtlas->texture->downloadContents(ae->x, ae->y, ae->sx, ae->sy, bytes, numBytes);
				
				result &= fwrite(bytes, numBytes, 1, file) == 1;
				
				delete[] bytes;
				
				if (result == false)
				{
					logDebug("saveCache: failed to save glyph data");
				}
			}
		}
	}
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
	
	if (result == false)
	{
		logDebug("saveCache: failed to save cache");
	}
	
	return result;
}

// -----

MsdfFontCacheElem::MsdfFontCacheElem()
	: m_filename()
	, m_glyphCache()
{
}

void MsdfFontCacheElem::free()
{
	delete m_glyphCache;
	m_glyphCache = nullptr;
	
	m_filename.clear();
}

void MsdfFontCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	m_filename = filename;
	
	m_glyphCache = new MsdfGlyphCache();
	m_glyphCache->load(filename);
}

//

void MsdfFontCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void MsdfFontCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

MsdfFontCacheElem & MsdfFontCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		MsdfFontCacheElem elem;
		
		elem.load(resolved_filename);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

#endif

//

BuiltinShaders::BuiltinShaders()
	: gaussianBlurH("engine/builtin-gaussian-h")
	, gaussianBlurV("engine/builtin-gaussian-v")
	, gaussianKernelBuffer()
	, colorMultiply("engine/builtin-colormultiply")
	, colorTemperature("engine/builtin-colortemperature")
	, textureSwizzle("engine/builtin-textureswizzle")
	, threshold("engine/builtin-threshold")
	, thresholdValue("engine/builtin-threshold-componentwise")
	, grayscaleLumi("engine/builtin-grayscale-lumi")
	, grayscaleWeights("engine/builtin-grayscale-weights")
	, hueAssign("engine/builtin-hue-assign")
	, hueShift("engine/builtin-hue-shift")
	, hqLine("engine/builtin-hq-line")
	, hqFilledTriangle("engine/builtin-hq-filled-triangle")
	, hqFilledCircle("engine/builtin-hq-filled-circle")
	, hqFilledRect("engine/builtin-hq-filled-rect")
	, hqFilledRoundedRect("engine/builtin-hq-filled-rounded-rect")
	, hqStrokeTriangle("engine/builtin-hq-stroked-triangle")
	, hqStrokedCircle("engine/builtin-hq-stroked-circle")
	, hqStrokedRect("engine/builtin-hq-stroked-rect")
	, hqStrokedRoundedRect("engine/builtin-hq-stroked-rounded-rect")
	, hqShadedTriangle("engine/builtin-hq-shaded-triangle")
	, msdfText("engine/builtin-msdf-text")
	, bitmappedText("engine/builtin-bitmapped-text")
{
}

BuiltinShaders::~BuiltinShaders()
{
	gaussianKernelBuffer.free();
}

//

const ShaderOutput * findShaderOutput(const char name)
{
	for (auto & output : g_shaderOutputs)
		if (output.name == name)
			return &output;
	
	return nullptr;
}
