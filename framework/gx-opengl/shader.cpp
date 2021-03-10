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

#include "internal.h"

uint32_t Shader::getProgram() const
{
	return m_cacheElem ? m_cacheElem->program : 0;
}

Shader::Shader()
{
	m_cacheElem = nullptr;
}

Shader::Shader(const char * name, const char * outputs)
{
	m_cacheElem = nullptr;

	const int name_length = (int)strlen(name);
	const int file_length = name_length + 3 + 1;
	
	char * vs = (char*)alloca(file_length * sizeof(char));
	char * ps = (char*)alloca(file_length * sizeof(char));
	
	memcpy(vs, name, name_length);
	strcpy(vs + name_length, ".vs");
	
	memcpy(ps, name, name_length);
	strcpy(ps + name_length, ".ps");
	
	load(name, vs, ps, outputs);
}

Shader::Shader(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	m_cacheElem = nullptr;

	load(name, filenameVs, filenamePs, outputs);
}

Shader::~Shader()
{
	if (globals.shader == this)
		clearShader();
}

void Shader::load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	if (outputs == nullptr)
		outputs = globals.shaderOutputs;
	
	m_cacheElem = &g_shaderCache.findOrCreate(name, filenameVs, filenamePs, outputs);
}

void Shader::reload()
{
	m_cacheElem->reload();
}

bool Shader::isValid() const
{
	return m_cacheElem && m_cacheElem->program != 0;
}

int Shader::getVersion() const
{
	return m_cacheElem ? m_cacheElem->version : 0;
}

bool Shader::getErrorMessages(std::vector<std::string> & errorMessages) const
{
	if (m_cacheElem && !m_cacheElem->errorMessages.empty())
	{
		errorMessages = m_cacheElem->errorMessages;
		return true;
	}
	else
	{
		return false;
	}
}

GxImmediateIndex Shader::getImmediateIndex(const char * name) const
{
	return glGetUniformLocation(getProgram(), name);
}

void Shader::getImmediateValuef(const GxImmediateIndex index, float * value) const
{
	glGetUniformfv(getProgram(), index, value);
	checkErrorGL();
}

std::vector<GxImmediateInfo> Shader::getImmediateInfos() const
{
	std::vector<GxImmediateInfo> result;
	
	if (isValid() == false)
		return result;
	
	GLsizei uniformCount = 0;
	glGetProgramiv(getProgram(), GL_ACTIVE_UNIFORMS, &uniformCount);
	checkErrorGL();

	result.resize(uniformCount);

	int count = 0;
	
	for (int i = 0; i < uniformCount; ++i)
	{
		auto & info = result[count];
		
		const GLsizei bufferSize = 256;
		char name[bufferSize];
		
		GLsizei length;
		GLint size;
		GLenum type;
		
		glGetActiveUniform(getProgram(), i, bufferSize, &length, &size, &type, name);
		checkErrorGL();
		
		const GLint location = glGetUniformLocation(getProgram(), name);
		checkErrorGL();
		
		if (type == GL_FLOAT)
			info.type = GX_IMMEDIATE_FLOAT;
		else if (type == GL_FLOAT_VEC2)
			info.type = GX_IMMEDIATE_VEC2;
		else if (type == GL_FLOAT_VEC3)
			info.type = GX_IMMEDIATE_VEC3;
		else if (type == GL_FLOAT_VEC4)
			info.type = GX_IMMEDIATE_VEC4;
		else
			continue;
		
		info.name = name;
		info.index = location;
		
		count++;
	}
	
	result.resize(count);
	
	return result;
}

std::vector<GxTextureInfo> Shader::getTextureInfos() const
{
	std::vector<GxTextureInfo> result;
	
	if (isValid() == false)
		return result;
	
	GLsizei uniformCount = 0;
	glGetProgramiv(getProgram(), GL_ACTIVE_UNIFORMS, &uniformCount);
	checkErrorGL();

	result.resize(uniformCount);

	int count = 0;
	
	for (int i = 0; i < uniformCount; ++i)
	{
		auto & info = result[count];
		
		const GLsizei bufferSize = 256;
		char name[bufferSize];
		
		GLsizei length;
		GLint size;
		GLenum type;
		
		glGetActiveUniform(getProgram(), i, bufferSize, &length, &size, &type, name);
		checkErrorGL();
		
		const GLint location = glGetUniformLocation(getProgram(), name);
		checkErrorGL();
		
		if (type == GL_SAMPLER_2D)
			info.type = GX_IMMEDIATE_TEXTURE_2D;
		else
			continue;
			
		info.name = name;
		info.index = location;
		
		count++;
	}
	
	result.resize(count);
	
	return result;
}

#define SET_UNIFORM(name, op) \
	if (getProgram()) \
	{ \
		Assert(globals.shader == this); \
		const GLint index = glGetUniformLocation(getProgram(), name); \
		if (index == -1) \
		{ \
			/*logDebug("couldn't find shader uniform %s", name);*/ \
		} \
		else \
		{ \
			op; \
		} \
	}
		
void Shader::setImmediate(const char * name, float x)
{
	SET_UNIFORM(name, glUniform1f(index, x));
	checkErrorGL();
}

void Shader::setImmediate(const char * name, float x, float y)
{
	SET_UNIFORM(name, glUniform2f(index, x, y));
	checkErrorGL();
}

void Shader::setImmediate(const char * name, float x, float y, float z)
{
	SET_UNIFORM(name, glUniform3f(index, x, y, z));
	checkErrorGL();
}

void Shader::setImmediate(const char * name, float x, float y, float z, float w)
{
	SET_UNIFORM(name, glUniform4f(index, x, y, z, w));
	checkErrorGL();
}

void Shader::setImmediate(GxImmediateIndex index, float x)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform1f(index, x);
	checkErrorGL();
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform2f(index, x, y);
	checkErrorGL();
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform3f(index, x, y, z);
	checkErrorGL();
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z, float w)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform4f(index, x, y, z, w);
	checkErrorGL();
}

void Shader::setImmediateFloatArray(const char * name, const float * values, const int numValues)
{
	SET_UNIFORM(name, glUniform1fv(index, numValues, values));
	checkErrorGL();
}

void Shader::setImmediateFloatArray(GxImmediateIndex index, const float * values, const int numValues)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform1fv(index, numValues, values);
	checkErrorGL();
}
	
void Shader::setImmediateMatrix4x4(const char * name, const float * matrix)
{
	SET_UNIFORM(name, glUniformMatrix4fv(index, 1, GL_FALSE, matrix));
	checkErrorGL();
}

void Shader::setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniformMatrix4fv(index, 1, GL_FALSE, matrix);
	checkErrorGL();
}

void Shader::setImmediateMatrix4x4Array(const char * name, const float * matrices, const int numMatrices)
{
	SET_UNIFORM(name, glUniformMatrix4fv(index, numMatrices, GL_FALSE, matrices));
	checkErrorGL();
}

void Shader::setImmediateMatrix4x4Array(GxImmediateIndex index, const float * matrices, const int numMatrices)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniformMatrix4fv(index, numMatrices, GL_FALSE, matrices);
	checkErrorGL();
}

void Shader::setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTexture(GxImmediateIndex index, int unit, GxTextureId texture, bool filtered, bool clamped)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform1i(index, unit);
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTextureCube(const char * name, int unit, GxTextureId texture, bool filtered)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTexture3d(const char * name, int unit, GxTextureId texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

#undef SET_UNIFORM

void Shader::setBuffer(const char * name, const ShaderBuffer & buffer)
{
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetUniformBlockIndex(program, name);
	checkErrorGL();
	
	if (index == GL_INVALID_INDEX)
		logWarning("unable to find uniform block index for %s", name);
	else
		setBuffer(index, buffer);
}

void Shader::setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer)
{
	fassert(globals.shader == this);

	glUniformBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.getOpenglBufferId());

	checkErrorGL();
}

void Shader::setBufferRw(const char * name, const ShaderBufferRw & buffer)
{
#if ENABLE_DESKTOP_OPENGL == 0
	AssertMsg(false, "not supported");
#else
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
	checkErrorGL();

	if (index == GL_INVALID_INDEX)
		logWarning("unable to find block index for %s", name);
	else
		setBufferRw(index, buffer);
#endif
}

void Shader::setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer)
{
#if ENABLE_DESKTOP_OPENGL == 0
	AssertMsg(false, "not supported");
#else
	fassert(globals.shader == this);

	glShaderStorageBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.getOpenglBufferId());

	checkErrorGL();
#endif
}

#endif
