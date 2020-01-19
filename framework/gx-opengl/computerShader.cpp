#include "framework.h"

#if ENABLE_OPENGL && ENABLE_OPENGL_COMPUTE_SHADER // todo : metal compute shader implementation

#include "internal.h"

uint32_t ComputeShader::getProgram() const
{
	return m_shader ? m_shader->program : 0;
}

ComputeShader::ComputeShader()
{
	m_shader = 0;
}

ComputeShader::ComputeShader(const char * filename, const int groupSx, const int groupSy, const int groupSz)
{
	m_shader = 0;

	load(filename, groupSx, groupSy, groupSz);
}

ComputeShader::~ComputeShader()
{
	if (globals.shader == this)
		clearShader();
}

void ComputeShader::load(const char * filename, const int groupSx, const int groupSy, const int groupSz)
{
	m_shader = &g_computeShaderCache.findOrCreate(filename, groupSx, groupSy, groupSz);
}

int ComputeShader::getVersion() const
{
	return m_shader ? m_shader->version : 0;
}

bool ComputeShader::getErrorMessages(std::vector<std::string> & errorMessages) const
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

int ComputeShader::getGroupSx() const
{
	return m_shader ? m_shader->groupSx : 0;
}

int ComputeShader::getGroupSy() const
{
	return m_shader ? m_shader->groupSy : 0;
}

int ComputeShader::getGroupSz() const
{
	return m_shader ? m_shader->groupSz : 0;
}

static int calcThreadSize(const int dispatchSize, const int groupSize)
{
	if (groupSize > 0)
		return (dispatchSize + groupSize - 1) / groupSize;
	else
		return 0;
}

int ComputeShader::toThreadSx(const int sx) const
{
	return calcThreadSize(sx, getGroupSx());
}

int ComputeShader::toThreadSy(const int sy) const
{
	return calcThreadSize(sy, getGroupSy());
}

int ComputeShader::toThreadSz(const int sz) const
{
	return calcThreadSize(sz, getGroupSz());
}

GxImmediateIndex ComputeShader::getImmediateIndex(const char * name)
{
	return glGetUniformLocation(getProgram(), name);
}

#define SET_UNIFORM(name, op) \
	if (getProgram()) \
	{ \
		setShader(*this); \
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

void ComputeShader::setImmediate(const char * name, float x)
{
	SET_UNIFORM(name, glUniform1f(index, x));
	checkErrorGL();
}

void ComputeShader::setImmediate(const char * name, float x, float y)
{
	SET_UNIFORM(name, glUniform2f(index, x, y));
	checkErrorGL();
}

void ComputeShader::setImmediate(const char * name, float x, float y, float z)
{
	SET_UNIFORM(name, glUniform3f(index, x, y, z));
	checkErrorGL();
}

void ComputeShader::setImmediate(const char * name, float x, float y, float z, float w)
{
	SET_UNIFORM(name, glUniform4f(index, x, y, z, w));
	checkErrorGL();
}

void ComputeShader::setImmediate(GxImmediateIndex index, float x, float y)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform2f(index, x, y);
	checkErrorGL();
}

void ComputeShader::setImmediate(GxImmediateIndex index, float x, float y, float z, float w)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform4f(index, x, y, z, w);
	checkErrorGL();
}

void ComputeShader::setImmediateMatrix4x4(const char * name, const float * matrix)
{
	SET_UNIFORM(name, glUniformMatrix4fv(index, 1, GL_FALSE, matrix));
	checkErrorGL();
}

void ComputeShader::setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniformMatrix4fv(index, 1, GL_FALSE, matrix);
	checkErrorGL();
}

void ComputeShader::setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamped)
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

void ComputeShader::setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamped)
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

void ComputeShader::setTextureRw(const char * name, int unit, GxTextureId texture, GLuint format, bool filtered, bool clamp)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glBindImageTexture(unit, texture, 0, false, 0, GL_READ_WRITE, format);
	checkErrorGL();
}

#undef SET_UNIFORM

void ComputeShader::setBuffer(const char * name, const ShaderBuffer & buffer)
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

void ComputeShader::setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer)
{
	fassert(globals.shader == this);

	glUniformBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void ComputeShader::setBufferRw(const char * name, const ShaderBufferRw & buffer)
{
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
	checkErrorGL();

	if (index == GL_INVALID_INDEX)
		logWarning("unable to find block index for %s", name);
	else
		setBufferRw(index, buffer);
}

void ComputeShader::setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer)
{
	fassert(globals.shader == this);

	glShaderStorageBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void ComputeShader::dispatch(const int dispatchSx, const int dispatchSy, const int dispatchSz)
{
	fassert(globals.shader == this);

	const int threadSx = toThreadSx(dispatchSx);
	const int threadSy = toThreadSy(dispatchSy);
	const int threadSz = toThreadSz(dispatchSz);

	glDispatchCompute(threadSx, threadSy, threadSz);
	checkErrorGL();

	// todo : let application insert barriers
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	checkErrorGL();
}

void ComputeShader::reload()
{
	m_shader->reload();
}

#endif
