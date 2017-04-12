#include <GL/glew.h>
#include "audio.h"
#include "data/engine/ShaderCommon.txt"
#include "image.h"
#include "internal.h"
#include "spriter.h"
#include "StringEx.h"

#if defined(WIN32)
	#include <Windows.h>
	#include <Pathcch.h>
	#include "FileStream.h"
	#include "StreamReader.h"
	#include "StreamWriter.h"
#endif

#if defined(DEBUG)
	#include "Timer.h"
#endif

Globals globals;

TextureCache g_textureCache;
ShaderCache g_shaderCache;
ComputeShaderCache g_computeShaderCache;
AnimCache g_animCache;
SpriterCache g_spriterCache;
SoundCache g_soundCache;
FontCache g_fontCache;
GlyphCache g_glyphCache;
UiCache g_uiCache;

// -----

void checkErrorGL_internal(const char * function, int line)
{
#if FRAMEWORK_ENABLE_GL_ERROR_LOG
	const GLenum error = glGetError();
	
	if (error != GL_NO_ERROR)
	{
		logError("%s: %d: OpenGL error: %x", function, line, error);

		#if 1
		static int skipCount = 1;
		if (skipCount-- <= 0)
			fassert(false);
		#endif
	}
#endif
}

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
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

void __stdcall debugOutputGL(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar * message,
	GLvoid * userParam)
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

class ScopedLoadTimer
{
public:
#if defined(DEBUG)
	const char * m_filename;
	uint64_t m_startTime;

	ScopedLoadTimer(const char * filename)
		: m_filename(filename)
	{
		logDebug("load %s [begin]", m_filename);

		m_startTime = g_TimerRT.TimeUS_get();
	}

	~ScopedLoadTimer()
	{
		const uint64_t endTime = g_TimerRT.TimeUS_get();

		logDebug("load %s [end] took %02.2fms", m_filename, (endTime - m_startTime) / 1000.f);
	}
#else
	ScopedLoadTimer(const char * filename)
	{
	}
#endif
};

//

void bindVsInputs(const VsInput * vsInputs, int numVsInputs, int stride)
{
	checkErrorGL();
	
	for (int i = 0; i < numVsInputs; ++i)
	{
		//logDebug("i=%d, id=%d, num=%d, type=%d, norm=%d, stride=%d, offset=%p\n", i, vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, stride, (void*)vsInputs[i].offset);
		
		glEnableVertexAttribArray(vsInputs[i].id);
		checkErrorGL();
		
		glVertexAttribPointer(vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, stride, (void*)(intptr_t)vsInputs[i].offset);
		checkErrorGL();
	}
}

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
		checkErrorGL();
		
		name.clear();
		textures = 0;
		sx = sy = 0;
		gridSx = gridSy = 0;
	}
}

#ifdef WIN32
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
		sprintf_s(hashName, sizeof(hashName), "fwc%08x.bin", hash);
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
#endif

void TextureCacheElem::load(const char * filename, int gridSx, int gridSy)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	name = filename;
	
	ImageData * imageData = 0;

#ifdef WIN32
	if (framework.cacheResourceData)
	{
		std::string cacheFilename = getCacheFilename(filename, true);

		if (FileStream::Exists(cacheFilename.c_str()))
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

				delete imageData;
				imageData = 0;
			}
		}
	}
#endif

	if (!imageData)
	{
		imageData = loadImage(filename);

	#ifdef WIN32
		if (framework.cacheResourceData && imageData)
		{
			std::string cacheFilename = getCacheFilename(filename, false);

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
			}
		}
	#endif
	}
	
	if (!imageData)
	{
		logError("failed to load %s (%dx%d)", filename, gridSx, gridSy);
	}
	else
	{
	#if 1
		ImageData * temp = imageFixAlphaFilter(imageData);
		delete imageData;
		imageData = temp;
	#endif

	#if 0
		ImageData * temp = imagePremultiplyAlpha(imageData);
		delete imageData;
		imageData = temp;
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
					//const int sourceOffset = sourceX + sourceY * imageData->sx;
					const int sourceOffset = sourceX + (imageData->sy - (sourceY + cellSy)) * imageData->sx;
					
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
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					checkErrorGL();

					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, imageData->sx);
					glTexImage2D(
						GL_TEXTURE_2D,
						0,
						GL_RGBA8,
						cellSx,
						cellSy,
						0,
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						source);
					
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					
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
			
			log("loaded %s (%dx%d)", filename, gridSx, gridSy);
		}
		
		delete imageData;
	}
}

void TextureCacheElem::reload()
{
	const std::string oldName = name;
	const int oldGridSx = gridSx;
	const int oldGridSy = gridSy;

	free();

	load(oldName.c_str(), oldGridSx, oldGridSy);
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

static void showShaderInfoLog(GLuint shader, const char * source)
{
#if FRAMEWORK_ENABLE_GL_ERROR_LOG
	GLint logSize = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

	char * log = new char[logSize];

	glGetShaderInfoLog(shader, logSize, &logSize, log);

	bool newLine = true;

	for (int line = 1; *source; )
	{
		if (newLine)
		{
			printf("%04d: ", line);
			newLine = false;
		}

		if (*source == '\n' || *source == '\r')
		{
			newLine = true;
			line++;
		}

		printf("%c", *source);

		source++;
	}

	logError("OpenGL shader compile failed:\n%s\n----\n%s", source, log);

	delete [] log;
	log = 0;
#endif
}

static void showProgramInfoLog(GLuint program)
{
	GLint logSize = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);

	char * log = new char[logSize];

	glGetProgramInfoLog(program, logSize, &logSize, log);

	logError("OpenGL program link failed:\n%s", log);

	delete [] log;
	log = 0;
}

static bool fileExists(const char * filename)
{
	const char * text = nullptr;

	FILE * file;

	if (fopen_s(&file, filename, "rb") == 0)
	{
		fclose(file);
		return true;
	}
	else if (framework.tryGetShaderSource(filename, text))
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool loadFileContents(const char * filename, bool normalizeLineEndings, char *& bytes, int & numBytes)
{
	bool result = true;

	bytes = 0;
	numBytes = 0;

	const char * text = nullptr;

	FILE * file = 0;

	if (fopen_s(&file, filename, "rb") == 0)
	{
		// load source from file

		fseek(file, 0, SEEK_END);
		numBytes = ftell(file);
		fseek(file, 0, SEEK_SET);

		bytes = new char[numBytes];

		if (fread(bytes, 1, numBytes, file) != (size_t)numBytes)
		{
			result = false;
		}

		fclose(file);
	}
	else if (framework.tryGetShaderSource(filename, text))
	{
		const size_t textLength = strlen(text);

		bytes = new char[textLength];
		numBytes = textLength;

		memcpy(bytes, text, textLength * sizeof(char));
	}
	else
	{
		result = false;
	}

	if (result)
	{
		if (normalizeLineEndings)
		{
			for (int i = 0; i < numBytes; ++i)
				if (bytes[i] == '\r')
					bytes[i] = '\n';
		}
	}
	else
	{
		if (bytes)
		{
			delete [] bytes;
			bytes = 0;
		}

		numBytes = 0;
	}

	return result;
}

static bool preprocessShader(const std::string & source, std::string & destination)
{
	bool result = true;

	std::vector<std::string> lines;

	splitString(source, lines, '\n');

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string & line = lines[i];
		const std::string trimmedLine = String::TrimLeft(lines[i]);

		const char * includeStr = "include ";

		if (strstr(trimmedLine.c_str(), includeStr) == trimmedLine.c_str())
		{
			const char * filename = trimmedLine.c_str() + strlen(includeStr);

			char * bytes;
			int numBytes;

			if (!loadFileContents(filename, true, bytes, numBytes))
			{
				logError("failed to load include file %s", filename);
				result = false;
			}
			else
			{
				std::string temp(bytes, numBytes);

				if (!preprocessShader(temp, destination))
				{
					result = false;
				}

				delete [] bytes;
				bytes = 0;
				numBytes = 0;
			}
		}
		else
		{
			destination.append(line);
			destination.append("\n");
		}
	}

	return result;
}

static bool loadShader(const char * filename, GLuint & shader, GLuint type, const char * defines)
{
	bool result = true;

	char * bytes;
	int numBytes;

	if (!loadFileContents(filename, true, bytes, numBytes))
	{
		result = false;
	}
	else
	{
		std::string source;
		std::string temp(bytes, numBytes);

		if (!preprocessShader(temp, source))
		{
			result = false;
		}
		else
		{
			//logDebug("shader source: %s", source.c_str());

			shader = glCreateShader(type);

		#if USE_LEGACY_OPENGL
			const GLchar * version = "#version 120\n#define _SHADER_ 1\n#define LEGACY_GL 1\n#define GLSL_VERSION 120";
        #elif OPENGL_VERSION == 410
            const GLchar * version = "#version 410\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 420\n";
		#elif OPENGL_VERSION == 430
			const GLchar * version = "#version 430\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 420\n";
		#else
			const GLchar * version = "#version 150\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 150";
		#endif

		#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
			const GLchar * debugs = "#define _SHADER_DEBUGGING_ 1\n";
		#else
			const GLchar * debugs = "#define _SHADER_DEBUGGING_ 0\n";
		#endif

			const GLchar * sourceData = (const GLchar*)source.c_str();
			const GLchar * sources[] = { version, debugs, defines, sourceData };

			glShaderSource(shader, sizeof(sources) / sizeof(sources[0]), sources, 0);
			checkErrorGL();

			delete [] bytes;
			bytes = 0;
			numBytes = 0;

			glCompileShader(shader);
			checkErrorGL();

			GLint success = GL_FALSE;

			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			checkErrorGL();

			if (success != GL_TRUE)
			{
				result = false;

				showShaderInfoLog(shader, source.c_str());
			}
		}
	}

	if (result)
	{
		log("loaded shader %s", filename);
	}
	else
	{
		logError("failed to load shader %s", filename);
	}

	return result;
}

// -----

ShaderCacheElem::ShaderCacheElem()
{
	program = 0;

	memset(params, -1, sizeof(params));
}

void ShaderCacheElem::free()
{
	if (program)
	{
		glDeleteProgram(program);
		program = 0;
	}

	memset(params, -1, sizeof(params));
}

void ShaderCacheElem::load(const char * _name, const char * filenameVs, const char * filenamePs)
{
	ScopedLoadTimer loadTimer(_name);

	free();
	
	bool result = true;
	
	name = _name;
	vs = filenameVs;
	ps = filenamePs;
	
	bool hasVs = fileExists(vs.c_str());
	bool hasPs = fileExists(ps.c_str());
	
	GLuint shaderVs = 0;
	GLuint shaderPs = 0;
	
	if (hasVs)
		result &= loadShader(vs.c_str(), shaderVs, GL_VERTEX_SHADER, "");
	else
	{
		logWarning("shader %s doesn't have a vertex shader. cannot load", _name);

		result &= false;
	}

	if (hasPs)
		result &= loadShader(ps.c_str(), shaderPs, GL_FRAGMENT_SHADER, "");
	
	if (result)
	{
		program = glCreateProgram();
		
		if (hasVs)
		{
			glAttachShader(program, shaderVs);
			checkErrorGL();
		}
		if (hasPs)
		{
			glAttachShader(program, shaderPs);
			checkErrorGL();
		}
		
		glBindAttribLocation(program, VS_POSITION,      "in_position4");
		glBindAttribLocation(program, VS_NORMAL,        "in_normal");
		glBindAttribLocation(program, VS_COLOR,         "in_color");
		glBindAttribLocation(program, VS_TEXCOORD,      "in_texcoord");
		glBindAttribLocation(program, VS_BLEND_INDICES, "in_skinningBlendIndices");
		glBindAttribLocation(program, VS_BLEND_WEIGHTS, "in_skinningBlendWeights");
		checkErrorGL();
		
		glLinkProgram(program);
		checkErrorGL();
		
		if (hasVs)
		{
			glDetachShader(program, shaderVs);
			checkErrorGL();
		}
		if (hasPs)
		{
			glDetachShader(program, shaderPs);
			checkErrorGL();
		}
		
		GLint success = GL_FALSE;
		
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		
		if (success != GL_TRUE)
		{
			result = false;
			
			showProgramInfoLog(program);
		}
		else
		{
			params[kSp_ModelViewMatrix].set(glGetUniformLocation(program, "ModelViewMatrix"));
			params[kSp_ModelViewProjectionMatrix].set(glGetUniformLocation(program, "ModelViewProjectionMatrix"));
			params[kSp_ProjectionMatrix].set(glGetUniformLocation(program, "ProjectionMatrix"));
			params[kSp_Texture].set(glGetUniformLocation(program, "texture0"));
			params[kSp_Params].set(glGetUniformLocation(program, "params"));

			// yay!
		}
	}
	
	if (shaderVs)
	{
		glDeleteShader(shaderVs);
		shaderVs = 0;
	}
	
	if (shaderPs)
	{
		glDeleteShader(shaderPs);
		shaderPs = 0;
	}
	
	if (result)
	{
		log("loaded shader program %s", name.c_str());
	}
	else
	{
		logError("failed to load shader program %s", name.c_str());
		
		free();
	}
}

void ShaderCacheElem::reload()
{
	const std::string oldName = name;
	const std::string oldVs = vs;
	const std::string oldPs = ps;

	load(oldName.c_str(), oldVs.c_str(), oldPs.c_str());
}

void ShaderCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void ShaderCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.reload();
	}
}

ShaderCacheElem & ShaderCache::findOrCreate(const char * name, const char * filenameVs, const char * filenamePs)
{
	Map::iterator i = m_map.find(name);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		ShaderCacheElem elem;
		
		elem.load(name, filenameVs, filenamePs);
		
		i = m_map.insert(Map::value_type(name, elem)).first;
		
		return i->second;
	}
}

// -----

ComputeShaderCacheElem::ComputeShaderCacheElem()
{
	groupSx = 0;
	groupSy = 0;
	groupSz = 0;

	program = 0;
}

void ComputeShaderCacheElem::free()
{
	if (program)
	{
		glDeleteProgram(program);
		program = 0;
	}
}

void ComputeShaderCacheElem::load(const char * _name, const int _groupSx, const int _groupSy, const int _groupSz)
{
	ScopedLoadTimer loadTimer(_name);

	free();

	bool result = true;

	name = _name;
	groupSx = _groupSx;
	groupSy = _groupSy;
	groupSz = _groupSz;

	GLuint shaderCs = 0;

	char defines[1024];
	sprintf_s(defines, sizeof(defines), "#define LOCAL_SIZE_X %d\n#define LOCAL_SIZE_Y %d\n#define LOCAL_SIZE_Z %d\n",
		groupSx,
		groupSy,
		groupSz);

	result &= loadShader(name.c_str(), shaderCs, GL_COMPUTE_SHADER, defines);

	if (result)
	{
		program = glCreateProgram();

		glAttachShader(program, shaderCs);
		checkErrorGL();

		glLinkProgram(program);
		checkErrorGL();

		glDetachShader(program, shaderCs);
		checkErrorGL();

		GLint success = GL_FALSE;

		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (success != GL_TRUE)
		{
			result = false;

			showProgramInfoLog(program);
		}
		else
		{
			// yay!
		}
	}

	if (shaderCs)
	{
		glDeleteShader(shaderCs);
		shaderCs = 0;
	}

	if (result)
	{
		log("loaded shader program %s", name.c_str());
	}
	else
	{
		logError("failed to load shader program %s", name.c_str());

		free();
	}
}

void ComputeShaderCacheElem::reload()
{
	const std::string oldName = name;

	load(oldName.c_str(), groupSx, groupSy, groupSz);
}

void ComputeShaderCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}

	m_map.clear();
}

void ComputeShaderCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.reload();
	}
}

ComputeShaderCacheElem & ComputeShaderCache::findOrCreate(const char * name, const int groupSx, const int groupSy, const int groupSz)
{
	Map::iterator i = m_map.find(name);

	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		ComputeShaderCacheElem elem;

		elem.load(name, groupSx, groupSy, groupSz);

		i = m_map.insert(Map::value_type(name, elem)).first;

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
void splitString(const std::string & str, std::vector<std::string> & result, Policy policy)
{
	int start = -1;
	
	for (size_t i = 0; i <= str.size(); ++i)
	{
		const char c = i < str.size() ? str[i] : policy.getBreakChar();
		
		if (start == -1)
		{
			// found start
			if (!policy.isBreak(c))
				start = i;
		}
		else if (policy.isBreak(c))
		{
			// found end
			result.push_back(str.substr(start, i - start));
			start = -1;
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

void splitString(const std::string & str, std::vector<std::string> & result)
{
	WhiteSpacePolicy policy;
	
	splitString<WhiteSpacePolicy>(str, result, policy);
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
	
	splitString<CharPolicy>(str, result, policy);
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
		AnimCacheElem elem;
		
		elem.load(name);
		
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
		log("loaded spriter %s", filename);
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
		SpriterCacheElem elem;
		
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
	ScopedLoadTimer loadTimer(filename);

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
				
				logDebug("%s: buffer=%x, bufferFormat=%x, samleData=%p, bufferSize=%d, sampleRate=%d",
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
	ScopedLoadTimer loadTimer(filename);

	free();
	
	if (FT_New_Face(globals.freeType, filename, 0, &face) != 0)
	{
		logError("%s: unable to open font", filename);
		face = 0;
	}
	else
	{
		log("loaded %s", filename);

		// fixme : this is a work around for FreeType returning monochrome data in FT_Load_Char, instead of the 8 gray scale
		// data it should be returning, when it find a stored glyph bitmap in the font itself. since we cannot directly upload
		// bit packed font data to OpenGL, we 'force' FreeType to always render the outline version instead, by setting
		// num_fixed_sizes to zero here
		face->num_fixed_sizes = 0;
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
			checkErrorGL();
		}
	}
	
	m_map.clear();
}

GlyphCacheElem & GlyphCache::findOrCreate(FT_Face face, int size, int c)
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

		if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) == 0)
		{
			// capture current OpenGL states before we change them
			
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

//

void UiCacheElem::free()
{
	map.clear();
}

void UiCacheElem::load(const char * filename)
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
		std::string line;
		
		int nameAlloc = 0;
		
		while (r.read(line))
		{
			if (line.size() == 0 || line[0] == '#')
			{
				// empty line or comment
				continue;
			}
			
			Dictionary d;
			
			if (!d.parse(line))
			{
				logError("%s: parse error: %s", filename, line.c_str());
			}
			else
			{
				std::string name = d.getString("name", "");
				
				if (name.empty())
				{
					char temp[32];
					sprintf_s(temp, sizeof(temp), "noname_%d", nameAlloc++);
					name = temp;
				}
				
				map[name] = d;
			}
		}
		
		log("loaded %s", filename);
	}
}

void UiCache::clear()
{
	m_map.clear();
}

void UiCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

UiCacheElem & UiCache::findOrCreate(const char * filename)
{
	Map::iterator i = m_map.find(filename);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		UiCacheElem elem;
		
		elem.load(filename);
		
		i = m_map.insert(Map::value_type(filename, elem)).first;
		
		return i->second;
	}
}

//

BuiltinShaders::BuiltinShaders()
	: gaussianBlurH("engine/builtin-gaussian-h")
	, gaussianBlurV("engine/builtin-gaussian-v")
	, colorMultiply("engine/builtin-colormultiply")
	, colorTemperature("engine/builtin-colortemperature")
	, hqLine("engine/builtin-hq-line")
	, hqFilledTriangle("engine/builtin-hq-filled-triangle")
	, hqFilledCircle("engine/builtin-hq-filled-circle")
	, hqFilledRect("engine/builtin-hq-filled-rect")
	, hqStrokeTriangle("engine/builtin-hq-stroked-triangle")
	, hqStrokedCircle("engine/builtin-hq-stroked-circle")
	, hqStrokedRect("engine/builtin-hq-stroked-rect")
	, invert("engine/builtin-invert")
{
}
