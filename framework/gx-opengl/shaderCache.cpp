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

#include "framework.h"

#if ENABLE_OPENGL

#include "data/engine/ShaderCommon.txt"
#include "internal.h"
#include "shaderBuilder.h"
#include "shaderCache.h"
#include "shaderPreprocess.h"
#include "StringEx.h"

ShaderCache g_shaderCache;
#if ENABLE_OPENGL_COMPUTE_SHADER
ComputeShaderCache g_computeShaderCache;
#endif

static void getShaderInfoLog(GLuint shader, const char * source, std::vector<std::string> & lines)
{
	GLint logSize = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

	char * log = new char[logSize];

	glGetShaderInfoLog(shader, logSize, &logSize, log);
	
	splitString(log, lines, '\n');
	
	delete [] log;
	log = 0;
}

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

static void getProgramInfoLog(GLuint program, std::string & line)
{
	GLint logSize = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
	
	line.resize(logSize);

	glGetProgramInfoLog(program, logSize, &logSize, &line[0]);
}

static void showProgramInfoLog(GLuint program)
{
#if FRAMEWORK_ENABLE_GL_ERROR_LOG
	std::string line;
	getProgramInfoLog(program, line);

	logError("OpenGL program link failed:\n%s", line.c_str());
#endif
}

static bool fileExists(const char * filename)
{
	const char * text = nullptr;

	const char * resolved_filename = framework.resolveResourcePath(filename);
	
	FILE * file;

	if (fopen_s(&file, resolved_filename, "rb") == 0)
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

static bool loadShader(const char * filename, GLuint & shader, GLuint type, const char * defines, std::vector<std::string> & errorMessages, std::vector<std::string> & includedFiles, const char * outputs)
{
	bool result = true;

	std::string source;
	
	if (!preprocessShaderFromFile(filename, source, kPreprocessShader_AddOpenglLineAndFileMarkers, errorMessages, includedFiles))
	{
		result = false;
	}
	else
	{
		if ((type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER) &&
			!buildOpenglText(source.c_str(),
				type == GL_VERTEX_SHADER   ? 'v' :
				type == GL_FRAGMENT_SHADER ? 'p' :
			#if ENABLE_OPENGL_COMPUTE_SHADER
				type == GL_COMPUTE_SHADER  ? 'c' :
			#endif
			 	'u',
				outputs, source))
		{
			result = false;
		}
		else
		{
			//printf("shader source:\n%s", source.c_str());

			shader = glCreateShader(type);
			checkErrorGL();
			
			if (shader == 0)
			{
				result = false;
				
			#if ENABLE_OPENGL_COMPUTE_SHADER
				if (type == GL_COMPUTE_SHADER)
					logError("compute shader creation failed. compute is possibly not supported?");
				else
			#endif
					logError("shader creation failed");
			}
			else
			{
				/*
					GLSL Version      OpenGL Version
					1.10              2.0
					1.20              2.1            <-- USE_LEGACY_OPENGL = OpenGL 2.1 with basic shader support
					1.30              3.0
					1.40              3.1
					1.50              3.2
					3.30              3.3
					4.00              4.0
					4.10              4.1            <-- OPENGL_VERSION 410 = Maximum version on OSX
					4.20              4.2
					4.30              4.3            <-- OPENGL_VERSION 430 = Safe to assume on Windows
					4.40              4.4
					4.50              4.5
				*/
				
			#if defined(IPHONEOS)
			// todo : add FRAMEWORK_USE_OPENGL_ES3 compile definition
				const GLchar * version = "#version 300 es\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 300\nprecision highp float;\n";
			#elif USE_LEGACY_OPENGL
				const GLchar * version = "#version 120\n#define _SHADER_ 1\n#define LEGACY_GL 1\n#define GLSL_VERSION 120\n";
			#elif FRAMEWORK_USE_OPENGL_ES
				const GLchar * version = "#version 300 es\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 420\nprecision mediump float;\n";
			#else
				#if OPENGL_VERSION == 410
					const GLchar * version = "#version 410\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 420\nprecision mediump float;\n";
				#elif OPENGL_VERSION == 430
					const GLchar * version = "#version 430\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 420\nprecision mediump float;\n";
				#else
					const GLchar * version = "#version 150\n#define _SHADER_ 1\n#define LEGACY_GL 0\n#define GLSL_VERSION 150\nprecision mediump float;\n";
				#endif
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

				glCompileShader(shader);
				checkErrorGL();

				GLint success = GL_FALSE;

				glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
				checkErrorGL();

				if (success != GL_TRUE)
				{
					result = false;

					showShaderInfoLog(shader, source.c_str());
					
					getShaderInfoLog(shader, source.c_str(), errorMessages);
				}
			}
		}
	}

	if (result)
	{
		logInfo("loaded shader %s", filename);
	}
	else
	{
		logError("failed to load shader %s", filename);
	}

	return result;
}

ShaderCacheElem::ShaderCacheElem()
{
	program = 0;
	version = 0;

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

void ShaderCacheElem::load(const char * _name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	ScopedLoadTimer loadTimer(_name);

	free();
	
	bool result = true;
	
	name = _name;
	vs = filenameVs;
	ps = filenamePs;
	this->outputs = outputs;
	
	version++;
	
	errorMessages.clear();
	includedFiles.clear();
	
	bool hasVs = fileExists(vs.c_str());
	bool hasPs = fileExists(ps.c_str());
	
	GLuint shaderVs = 0;
	GLuint shaderPs = 0;
	
	if (hasVs)
		result &= loadShader(vs.c_str(), shaderVs, GL_VERTEX_SHADER, "", errorMessages, includedFiles, "");
	else
	{
		errorMessages.push_back(String::Format("shader %s doesn't have a vertex shader. cannot load", _name));
		logWarning("shader %s doesn't have a vertex shader. cannot load", _name);
		result &= false;
	}

	if (hasPs)
	{
		if (result)
		{
			result &= loadShader(ps.c_str(), shaderPs, GL_FRAGMENT_SHADER, "", errorMessages, includedFiles, outputs);
		}
	}
	
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
		glBindAttribLocation(program, VS_TEXCOORD0,     "in_texcoord0");
		glBindAttribLocation(program, VS_TEXCOORD1,     "in_texcoord1");
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
			
			std::string log;
			getProgramInfoLog(program, log);
			errorMessages.push_back("program log:");
			errorMessages.push_back(log);
			
			showProgramInfoLog(program);
		}
		else
		{
			params[kSp_ModelViewMatrix].set(glGetUniformLocation(program, "ModelViewMatrix"));
			params[kSp_ModelViewProjectionMatrix].set(glGetUniformLocation(program, "ModelViewProjectionMatrix"));
			params[kSp_ProjectionMatrix].set(glGetUniformLocation(program, "ProjectionMatrix"));
			params[kSp_SkinningMatrices].set(glGetUniformLocation(program, "skinningMatrices"));
			params[kSp_Texture].set(glGetUniformLocation(program, "source"));
			params[kSp_Params].set(glGetUniformLocation(program, "params"));
			params[kSp_ShadingParams].set(glGetUniformLocation(program, "shadingParams"));
			params[kSp_GradientInfo].set(glGetUniformLocation(program, "gradientInfo"));
			params[kSp_GradientMatrix].set(glGetUniformLocation(program, "gmat"));
			params[kSp_TextureMatrix].set(glGetUniformLocation(program, "tmat"));

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
		logInfo("loaded shader program %s", name.c_str());
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
	const std::string oldOutputs = outputs;

	load(oldName.c_str(), oldVs.c_str(), oldPs.c_str(), oldOutputs.c_str());
}

bool ShaderCacheElem::hasIncludedFile(const char * filename)
{
	for (auto & includedFile : includedFiles)
		if (filename == includedFile)
			return true;
	
	return false;
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

void ShaderCache::handleSourceChanged(const char * name)
{
	for (auto & shaderCacheItr : m_map)
	{
		ShaderCacheElem & cacheElem = shaderCacheItr.second;
		
		if (name == cacheElem.vs || name == cacheElem.ps || cacheElem.hasIncludedFile(name))
		{
			cacheElem.reload();
			
			if (globals.shader != nullptr && globals.shader->getOpenglProgram() == cacheElem.program)
			{
				clearShader();
			}
		}
	}
}

ShaderCacheElem & ShaderCache::findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	Key key;
	key.name = name;
	key.outputs = outputs;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		ShaderCacheElem elem;
		
		elem.load(name, filenameVs, filenamePs, outputs);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}

//

#if ENABLE_OPENGL_COMPUTE_SHADER

ComputeShaderCacheElem::ComputeShaderCacheElem()
{
	groupSx = 0;
	groupSy = 0;
	groupSz = 0;

	program = 0;
	
	version = 0;
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
	
	version++;
	
	errorMessages.clear();
	includedFiles.clear();

	GLuint shaderCs = 0;

	char defines[1024];
	sprintf_s(defines, sizeof(defines), "#define LOCAL_SIZE_X %d\n#define LOCAL_SIZE_Y %d\n#define LOCAL_SIZE_Z %d\n",
		groupSx,
		groupSy,
		groupSz);

	result &= loadShader(name.c_str(), shaderCs, GL_COMPUTE_SHADER, defines, errorMessages, includedFiles, "");

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
		logInfo("loaded shader program %s", name.c_str());
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

void ComputeShaderCache::handleSourceChanged(const char * name)
{
	for (auto & shaderCacheItr : m_map)
	{
		ComputeShaderCacheElem & cacheElem = shaderCacheItr.second;
		
		if (name == cacheElem.name || cacheElem.hasIncludedFile(name))
		{
			cacheElem.reload();
			
			if (globals.shader != nullptr && globals.shader->getOpenglProgram() == cacheElem.program)
			{
				clearShader();
			}
		}
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

bool ComputeShaderCache::hasIncludedFile(const char * filename)
{
	for (auto & includedFile : includedFiles)
		if (filename == includedFile)
			return true;
	
	return false;
}

#endif

#endif
