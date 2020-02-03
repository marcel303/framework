#import "metal.h"
#import "shader.h"
#import "shaderBuilder.h"
#import "shaderPreprocess.h"

#include <assert.h>
#define Assert assert

//

id <MTLDevice> metal_get_device();

//

class ShaderCache
{
	ShaderCacheElem m_cacheElem;
	
public:
	ShaderCacheElem & findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs);
};

ShaderCache g_shaderCache;

//

#include "mesh.h"

ShaderCacheElem & ShaderCache::findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	static bool init = false;
	
	if (init == false)
	{
		init = true;
		
		@autoreleasepool
		{
			id <MTLDevice> device = metal_get_device();
			
			std::vector<std::string> errorMessages;
			std::string shaderVs_opengl;
			std::string shaderPs_opengl;
			
			preprocessShaderFromFile("testShader.vs", shaderVs_opengl, 0, errorMessages);
			preprocessShaderFromFile("testShader.ps", shaderPs_opengl, 0, errorMessages);
			
			std::string shaderVs;
			std::string shaderPs;
			
			buildMetalText(shaderVs_opengl.c_str(), 'v', shaderVs);
			buildMetalText(shaderPs_opengl.c_str(), 'p', shaderPs);

			printf("vs text:\n%s", shaderVs.c_str());
			
			printf("\n\n\n");
			
			printf("ps text:\n%s", shaderPs.c_str());
			
			NSError * error = nullptr;
			
			id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:shaderVs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_vs == nullptr && error != nullptr)
				NSLog(@"%@", error);
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:shaderPs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
				NSLog(@"%@", error);
		
			id <MTLFunction> vs = [library_vs newFunctionWithName:@"shader_main"];
			id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];
			
			// get reflection info for this shader
			
			MTLRenderPipelineDescriptor * pipelineDescriptor = [[MTLRenderPipelineDescriptor new] autorelease];
			pipelineDescriptor.label = @"reflection pipeline";
			pipelineDescriptor.vertexFunction = vs;
			pipelineDescriptor.fragmentFunction = ps;
			pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
			
			MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
			
			for (int i = 0; i < renderState.vertexInputCount; ++i)
			{
				auto & e = renderState.vertexInputs[i];
				auto * a = vertexDescriptor.attributes[e.id];
				
				MTLVertexFormat metalFormat = MTLVertexFormatInvalid;
				
				if (e.type == GX_ELEMENT_FLOAT32)
				{
					if (e.numComponents == 1)
						metalFormat = MTLVertexFormatFloat;
					else if (e.numComponents == 2)
						metalFormat = MTLVertexFormatFloat2;
					else if (e.numComponents == 3)
						metalFormat = MTLVertexFormatFloat3;
					else if (e.numComponents == 4)
						metalFormat = MTLVertexFormatFloat4;
				}
				
				Assert(metalFormat != MTLVertexFormatInvalid);
				if (metalFormat != MTLVertexFormatInvalid)
				{
					a.format = metalFormat;
					a.offset = e.offset;
				}
			}
		
			vertexDescriptor.layouts[0].stride = renderState.vertexStride;
			vertexDescriptor.layouts[0].stepRate = 1;
			vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
			pipelineDescriptor.vertexDescriptor = vertexDescriptor;
			
			const MTLPipelineOption pipelineOptions = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
			
			MTLRenderPipelineReflection * reflection;
			
			id <MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor options:pipelineOptions reflection:&reflection error:&error];
			
			[pipelineState release];
			
		#if 0
			NSLog(@"library_vs retain count: %lu", [library_vs retainCount]);
			NSLog(@"library_ps retain count: %lu", [library_ps retainCount]);
			[library_vs release];
			[library_ps release];
			NSLog(@"library_vs retain count: %lu", [library_vs retainCount]);
			NSLog(@"library_ps retain count: %lu", [library_ps retainCount]);
		#endif
			
			m_cacheElem.init(reflection);
			m_cacheElem.vs = vs;
			m_cacheElem.ps = ps;
		}
	}
	
	return m_cacheElem;
}

//

static ShaderCacheElem::UniformInfo & getUniformInfo(ShaderCacheElem & cacheElem, const int index, const int type, const int numElems)
{
	Assert(index >= 0 && index < cacheElem.uniformInfos.size());
	auto & info = cacheElem.uniformInfos[index];
	Assert(info.elemType == type && info.numElems == numElems);
	return info;
}

template <typename T>
T * getVsUniformPtr(ShaderCacheElem & cacheElem, const int offset)
{
	return (T*)(((uint8_t*)cacheElem.vsUniformData) + offset);
}

template <typename T>
T * getPsUniformPtr(ShaderCacheElem & cacheElem, const int offset)
{
	return (T*)(((uint8_t*)cacheElem.psUniformData) + offset);
}

Shader::Shader(const char * name)
{
	m_cacheElem = &g_shaderCache.findOrCreate(name, nullptr, nullptr, "c");
}

GxImmediateIndex Shader::getImmediate(const char * name)
{
	for (size_t i = 0; i < m_cacheElem->uniformInfos.size(); ++i)
		if (m_cacheElem->uniformInfos[i].name == name)
			return i;
	return -1;
}

void Shader::setImmediate(const char * name, float x)
{
	const GxImmediateIndex index = getImmediate(name);
	
	setImmediate(index, x);
}

void Shader::setImmediate(const char * name, float x, float y)
{
	const GxImmediateIndex index = getImmediate(name);
	
	setImmediate(index, x, y);
}

void Shader::setImmediate(const char * name, float x, float y, float z)
{
	const GxImmediateIndex index = getImmediate(name);
	
	setImmediate(index, x, y, z);
}

void Shader::setImmediate(const char * name, float x, float y, float z, float w)
{
	const GxImmediateIndex index = getImmediate(name);
	
	setImmediate(index, x, y, z, w);
}

void Shader::setImmediate(GxImmediateIndex index, float x)
{
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsOffset);
			dst[0] = x;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psOffset);
			dst[0] = x;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y)
{
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 2);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsOffset);
			dst[0] = x;
			dst[1] = y;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psOffset);
			dst[0] = x;
			dst[1] = y;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z)
{
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 3);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z, float w)
{
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 4);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}
	}
}

void Shader::setImmediateMatrix4x4(const char * name, const float * matrix)
{
	const GxImmediateIndex index = getImmediate(name);
	
	setImmediateMatrix4x4(index, matrix);
}

void Shader::setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix)
{
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'm', 16);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsOffset);
			memcpy(dst, matrix, 16 * sizeof(float));
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psOffset);
			memcpy(dst, matrix, 16 * sizeof(float));
		}
	}
}
