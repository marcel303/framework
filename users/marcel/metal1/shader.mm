#import "shader.h"

#include <assert.h> // todo : use framework assert
#define Assert assert

ShaderCacheElem::UniformInfo & Shader::getUniformInfo(const int index, const int type, const int numElems)
{
	Assert(index >= 0 && index < m_cacheElem.uniformInfos.size());
	auto & info = m_cacheElem.uniformInfos[index];
	Assert(info.elemType == type && info.numElems == numElems);
	return info;
}

GxImmediateIndex Shader::getImmediate(const char * name)
{
	for (size_t i = 0; i < m_cacheElem.uniformInfos.size(); ++i)
		if (m_cacheElem.uniformInfos[i].name == name)
			return i;
	return -1;
}

void Shader::setImmediate(const char * name, float x)
{
	const GxImmediateIndex index = getImmediate(name);
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(index, 'f', 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(info.vsOffset);
			dst[0] = x;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(info.psOffset);
			dst[0] = x;
		}
	}
}

void Shader::setImmediate(const char * name, float x, float y)
{

}

void Shader::setImmediate(const char * name, float x, float y, float z)
{

}

void Shader::setImmediate(const char * name, float x, float y, float z, float w)
{
	const GxImmediateIndex index = getImmediate(name);
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(index, 'f', 4);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(info.vsOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(info.psOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x)
{

}

void Shader::setImmediate(GxImmediateIndex index, float x, float y)
{

}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z)
{

}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z, float w)
{

}

void Shader::setImmediateMatrix4x4(const char * name, const float * matrix)
{

}

void Shader::setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix)
{

}
