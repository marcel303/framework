#import "shader.h"

#include <assert.h> // todo : use framework assert
#define Assert assert

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
		auto & info = m_cacheElem.uniformInfos[index];
		
		Assert(info.elemType == 'f' && info.numElems == 1);
		
		if (info.vsOffset != -1)
		{

		}
		
		if (info.psOffset != -1)
		{

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
