#include "framework.h"

#if ENABLE_OPENGL

#include "internal.h"

extern std::string s_shaderOutputs; // todo : cleanup

Shader::Shader()
{
	m_shader = 0;
}

Shader::Shader(const char * name, const char * outputs)
{
	m_shader = 0;

	const int name_length = strlen(name);
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
	m_shader = 0;

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
		outputs = s_shaderOutputs.empty() ? "c" : s_shaderOutputs.c_str();
	
	m_shader = &g_shaderCache.findOrCreate(name, filenameVs, filenamePs, outputs);
}

bool Shader::isValid() const
{
	return m_shader != 0 && m_shader->program != 0;
}

GxShaderId Shader::getProgram() const
{
	return m_shader ? m_shader->program : 0;
}

int Shader::getVersion() const
{
	return m_shader ? m_shader->version : 0;
}

bool Shader::getErrorMessages(std::vector<std::string> & errorMessages) const
{
	if (m_shader && !m_shader->errorMessages.empty())
	{
		errorMessages = m_shader->errorMessages;
		return true;
	}
	else
	{
		return false;
	}
}

GxImmediateIndex Shader::getImmediateIndex(const char * name)
{
	return glGetUniformLocation(getProgram(), name);
}

void Shader::getImmediateValuef(const GxImmediateIndex index, float * value)
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

void Shader::setImmediateMatrix4x4Array(GxImmediateIndex index, const float * matrices, const int numMatrices)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniformMatrix4fv(index, numMatrices, GL_FALSE, matrices);
	checkErrorGL();
}

void Shader::setTextureUnit(const char * name, int unit)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();
}

void Shader::setTextureUnit(GxImmediateIndex index, int unit)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform1i(index, unit);
	checkErrorGL();
}

void Shader::setTexture(const char * name, int unit, GxTextureId texture)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE0);
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

void Shader::setTextureUniform(GxImmediateIndex index, int unit, GxTextureId texture)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform1i(index, unit);
	checkErrorGL();
	
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void Shader::setTextureArray(const char * name, int unit, GxTextureId texture)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
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

void Shader::setTextureCube(const char * name, int unit, GxTextureId texture)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
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
		logWarning("unable to find block index for %s", name);
	else
		setBuffer(index, buffer);
}

void Shader::setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer)
{
	fassert(globals.shader == this);

	glUniformBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void Shader::setBufferRw(const char * name, const ShaderBufferRw & buffer)
{
#if ENABLE_DESKTOP_OPENGL == 0
	AssertMsg(false, "not supported", 0);
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
	AssertMsg(false, "not supported", 0);
#else
	fassert(globals.shader == this);

	glShaderStorageBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
#endif
}

void Shader::reload()
{
	m_shader->reload();
}

#endif
