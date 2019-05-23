/*
	Copyright (C) 2017 Marcel Smit
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

#import "framework.h"

#if ENABLE_METAL

#import "internal.h"
#import "metal.h"
#import "shader.h"
#import "shaderBuilder.h"
#import "shaderPreprocess.h"
#import "texture.h"
#import "window_data.h"

// todo : assert the shader is the active shader when setting immediates

extern MetalWindowData * activeWindowData;

//

id <MTLDevice> metal_get_device();

//

ShaderCache g_shaderCache;

//

ShaderCacheElem & ShaderCache::findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	Key key;
	key.name = name;
	key.outputs = outputs;
	
	auto i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return *i->second;
	}
	else
	{
		ShaderCacheElem_Metal * elem = new ShaderCacheElem_Metal();
		
		@autoreleasepool
		{
			id <MTLDevice> device = metal_get_device();
			
			NSError * error = nullptr;
			
			std::vector<std::string> errorMessages;
			std::string shaderVs_opengl;
			std::string shaderPs_opengl;
			
			preprocessShaderFromFile(filenameVs, shaderVs_opengl, 0, errorMessages);
			preprocessShaderFromFile(filenamePs, shaderPs_opengl, 0, errorMessages);
			
			std::string shaderVs;
			std::string shaderPs;
			
			buildMetalText(shaderVs_opengl.c_str(), 'v', outputs, shaderVs);
			buildMetalText(shaderPs_opengl.c_str(), 'p', outputs, shaderPs);
			
			id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:shaderVs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_vs == nullptr && error != nullptr)
			{
				printf("vs text:\n%s", shaderVs.c_str());
				NSLog(@"%@", error);
			}
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:shaderPs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
			{
				printf("ps text:\n%s", shaderPs.c_str());
				NSLog(@"%@", error);
			}
			
			id <MTLFunction> vs = [library_vs newFunctionWithName:@"shader_main"];
			id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];
			
			// get reflection info for this shader
			
			MTLRenderPipelineDescriptor * pipelineDescriptor = [[MTLRenderPipelineDescriptor new] autorelease];
			pipelineDescriptor.label = @"reflection pipeline";
			pipelineDescriptor.vertexFunction = vs;
			pipelineDescriptor.fragmentFunction = ps;
			pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
			
			MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
			
		// fixme : this is duplicated code. perhaps we should enforce everything is set
		//         before the shader is set. when a shader is set, construct the pipeline state
		//         only thing allowed after a shader is set is set to immediates, and to
		//         do draw calls
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
				else if (e.type == GX_ELEMENT_UINT8)
				{
					if (e.numComponents == 1)
						metalFormat = e.normalize ? MTLVertexFormatUCharNormalized : MTLVertexFormatUChar;
					else if (e.numComponents == 2)
						metalFormat = e.normalize ? MTLVertexFormatUChar2Normalized : MTLVertexFormatUChar2;
					else if (e.numComponents == 3)
						metalFormat = e.normalize ? MTLVertexFormatUChar3Normalized : MTLVertexFormatUChar3;
					else if (e.numComponents == 4)
						metalFormat = e.normalize ? MTLVertexFormatUChar4Normalized : MTLVertexFormatUChar4;
				}
				
				Assert(metalFormat != MTLVertexFormatInvalid);
				if (metalFormat != MTLVertexFormatInvalid)
				{
					a.format = metalFormat;
					a.offset = e.offset;
					a.bufferIndex = 0;
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
			
			elem->init(reflection);
			elem->vs = vs;
			elem->ps = ps;
		}
		
		m_map[key] = elem;
		
		return *elem;
	}
}

//

static ShaderCacheElem_Metal::UniformInfo & getUniformInfo(ShaderCacheElem_Metal & cacheElem, const int index, const int type, const int numElems)
{
	Assert(index >= 0 && index < cacheElem.uniformInfos.size());
	auto & info = cacheElem.uniformInfos[index];
	Assert(info.elemType == type && info.numElems == numElems);
	return info;
}

template <typename T>
T * getVsUniformPtr(ShaderCacheElem_Metal & cacheElem, const int offset)
{
	return (T*)(((uint8_t*)cacheElem.vsUniformData) + offset);
}

template <typename T>
T * getPsUniformPtr(ShaderCacheElem_Metal & cacheElem, const int offset)
{
	return (T*)(((uint8_t*)cacheElem.psUniformData) + offset);
}

Shader::Shader()
{
}

Shader::Shader(const char * name, const char * outputs)
{
	const std::string vs = std::string(name) + ".vs";
	const std::string ps = std::string(name) + ".ps";
	
	load(name, vs.c_str(), ps.c_str(), outputs);
}

Shader::Shader(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	load(name, filenameVs, filenamePs, outputs);
}

Shader::~Shader()
{
	if (globals.shader == this)
		clearShader();
}

extern std::string s_shaderOutputs; // todo : cleanup

void Shader::load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	if (outputs == nullptr)
		outputs = s_shaderOutputs.c_str();
	
	m_cacheElem = static_cast<ShaderCacheElem_Metal*>(&g_shaderCache.findOrCreate(name, filenameVs, filenamePs, outputs));
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

void Shader::setImmediateMatrix4x4Array(GxImmediateIndex index, const float * matrices, const int numMatrices)
{
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'm', 16 * numMatrices);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsOffset);
			memcpy(dst, matrices, 16 * sizeof(float) * numMatrices);
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psOffset);
			memcpy(dst, matrices, 16 * sizeof(float) * numMatrices);
		}
	}
}

#define not_implemented Assert(false) // todo : implement shader stubs

inline int getTextureIndex(const ShaderCacheElem_Metal & elem, const char * name)
{
	for (size_t i = 0; i < elem.textureInfos.size(); ++i)
		if (elem.textureInfos[i].name == name)
			return i;
	return -1;
}

void Shader::setTextureUnit(const char * name, int unit)
{
	not_implemented;
}

void Shader::setTextureUnit(GxImmediateIndex index, int unit)
{
	not_implemented;
}

void Shader::setTexture(const char * name, int unit, GxTextureId texture)
{
	const int index = getTextureIndex(*m_cacheElem, name);
	
	if (index >= 0)
	{
		setTextureUniform(index, unit, texture);
	}
}

void Shader::setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	not_implemented;
	
	setTexture(name, unit, texture);
}

void Shader::setTextureUniform(GxImmediateIndex index, int unit, GxTextureId texture)
{
	not_implemented;
	
	Assert(index >= 0 && index < m_cacheElem->textureInfos.size());
	auto & info = m_cacheElem->textureInfos[index];
	
	auto i = s_textures.find(texture);
	Assert(i != s_textures.end());

	if (i != s_textures.end())
	{
		auto metal_texture = i->second;
		
		if (info.vsOffset >= 0 && info.vsOffset < ShaderCacheElem_Metal::kMaxVsTextures)
			m_cacheElem->vsTextures[info.vsOffset] = metal_texture;
		if (info.psOffset >= 0 && info.psOffset < ShaderCacheElem_Metal::kMaxPsTextures)
			m_cacheElem->psTextures[info.psOffset] = metal_texture;
	}
}

void Shader::setTextureArray(const char * name, int unit, GxTextureId texture)
{
	not_implemented;
}

void Shader::setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	not_implemented;
}

void Shader::setTextureCube(const char * name, int unit, GxTextureId texture)
{
	not_implemented;
}

void Shader::setBuffer(const char * name, const ShaderBuffer & buffer)
{
	not_implemented;
}

void Shader::setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer)
{
	not_implemented;
}

void Shader::setBufferRw(const char * name, const ShaderBufferRw & buffer)
{
	not_implemented;
}

void Shader::setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer)
{
	not_implemented;
}

const ShaderCacheElem & Shader::getCacheElem() const
{
	return *m_cacheElem;
}

#endif
