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

#include "mesh.h"

#if 1 // todo : move elsewhere

static const char * s_shaderVs = R"SHADER(

	#include <metal_stdlib>

	using namespace metal;

	struct ShaderInputs
	{
		float4 position [[attribute(0)]];
		float3 normal [[attribute(1)]];
		float4 color [[attribute(2)]];
		float2 texcoord [[attribute(3)]];
	};

	struct ShaderOutputs
	{
		float4 position [[position]];
		float4 color;
		float2 texcoord;
	};

	struct ShaderUniforms
	{
		float4x4 ProjectionMatrix;
		float4x4 ModelViewMatrix;
		float4x4 ModelViewProjectionMatrix;
	};

	#define unpackPosition() inputs.position

	vertex ShaderOutputs shader_main(
		ShaderInputs inputs [[stage_in]],
		constant ShaderUniforms & uniforms [[buffer(1)]])
	{
		ShaderOutputs outputs;
		
		outputs.position = uniforms.ModelViewProjectionMatrix * unpackPosition();
		//outputs.position = uniforms.ModelViewMatrix * inputs.position;
		//outputs.position = uniforms.ProjectionMatrix * float4(inputs.position.xy, 2, 1);
		outputs.color = inputs.color;
		outputs.texcoord = inputs.texcoord;
		
		return outputs;
	}

)SHADER";

static const char * s_shaderPs = R"SHADER(

	#include <metal_stdlib>

	using namespace metal;

	struct ShaderInputs
	{
		float4 position [[position]];
		float4 color;
		float2 texcoord;
	};

	struct ShaderUniforms
	{
		float4 params;
		float4 imms;
	};

	fragment float4 shader_main(
		ShaderInputs inputs [[stage_in]],
		constant ShaderUniforms & uniforms [[buffer(0)]],
		texture2d<float> textureResource [[texture(0)]])
	{
		float4 color = inputs.color;
		
		if (uniforms.params.x != 0.0)
		{
			constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
			
			color *= textureResource.sample(textureSampler, inputs.texcoord);
		}
		
		color += uniforms.imms;
		
		return color;
	}

)SHADER";

#endif

ShaderCacheElem & ShaderCache::findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	auto i = m_map.find(name);
	
	if (i != m_map.end())
	{
		return *i->second;
	}
	else
	{
		ShaderCacheElem * elem = new ShaderCacheElem();
		
		@autoreleasepool
		{
			id <MTLDevice> device = metal_get_device();
			
			NSError * error = nullptr;
			
		#if 1
			std::vector<std::string> errorMessages;
			std::string shaderVs_opengl;
			std::string shaderPs_opengl;
			
			preprocessShaderFromFile(filenameVs, shaderVs_opengl, 0, errorMessages);
			preprocessShaderFromFile(filenamePs, shaderPs_opengl, 0, errorMessages);
			
			std::string shaderVs;
			std::string shaderPs;
			
			buildMetalText(shaderVs_opengl.c_str(), 'v', shaderVs);
			buildMetalText(shaderPs_opengl.c_str(), 'p', shaderPs);

			printf("vs text:\n%s", shaderVs.c_str());
			
			printf("\n\n\n");
			
			printf("ps text:\n%s", shaderPs.c_str());
			
			id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:shaderVs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_vs == nullptr && error != nullptr)
				NSLog(@"%@", error);
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:shaderPs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
				NSLog(@"%@", error);
		#else
			id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:s_shaderVs encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_vs == nullptr && error != nullptr)
				NSLog(@"%@", error);
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:s_shaderPs encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
				NSLog(@"%@", error);
		#endif
		
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
		
		m_map[name] = elem;
		
		return *elem;
	}
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

Shader::Shader()
{
}

Shader::Shader(const char * name, const char * outputs)
{
	const std::string vs = std::string(name) + ".vs";
	const std::string ps = std::string(name) + ".ps";
	
	m_cacheElem = &g_shaderCache.findOrCreate(name, vs.c_str(), ps.c_str(), outputs);
}

Shader::Shader(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
// todo : s_shaderOutputs
	//if (outputs == nullptr)
	//	outputs = s_shaderOutputs.c_str();
	
	load(name, filenameVs, filenamePs, outputs);
}

Shader::~Shader()
{
}

void Shader::load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs)
{
	m_cacheElem = &g_shaderCache.findOrCreate(name, filenameVs, filenamePs, outputs);
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

#define not_implemented Assert(false) // todo : implement shader stubs

inline int getTextureIndex(const ShaderCacheElem & elem, const char * name)
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

void Shader::setTexture(const char * name, int unit, GxTextureId textureId)
{
	const int index = getTextureIndex(*m_cacheElem, name);
	
	if (index >= 0)
	{
		auto & info = m_cacheElem->textureInfos[index];
		
		auto i = s_textures.find(textureId);
		Assert(i != s_textures.end());
		
		if (i != s_textures.end())
		{
			auto texture = i->second;
			
			if (info.vsOffset >= 0)
				[activeWindowData->encoder setFragmentTexture:texture atIndex:info.vsOffset];
			if (info.psOffset >= 0)
				[activeWindowData->encoder setFragmentTexture:texture atIndex:info.psOffset];
		}
	}
}

void Shader::setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	not_implemented;
}

void Shader::setTextureUniform(GxImmediateIndex index, int unit, GxTextureId texture)
{
	not_implemented;
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

#endif
