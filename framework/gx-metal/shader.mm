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

#import "framework.h"

#if ENABLE_METAL

#import "internal.h"
#import "metal.h"
#import "shaderBuilder.h"
#import "shaderPreprocess.h"
#import "texture.h"
#import "window_data.h"

extern MetalWindowData * activeWindowData;

//

id <MTLDevice> metal_get_device();

//

ShaderCache g_shaderCache;

//

void ShaderCacheElem_Metal::init(MTLRenderPipelineReflection * reflection)
{
	// cache uniform offsets
	
	if (reflection != nullptr)
	{
		for (MTLArgument * arg in reflection.vertexArguments)
		{
			if (arg.type == MTLArgumentTypeBuffer && arg.bufferDataType == MTLDataTypeStruct && [arg.name isEqualToString:@"inputs"])
			{
				//logDebug("found inputs");
			}
			else if (arg.type == MTLArgumentTypeTexture)
			{
				//logDebug("found texture");
				addTexture(arg, 'v');
			}
			else if (arg.type == MTLArgumentTypeBuffer && arg.bufferDataType == MTLDataTypeStruct && [arg.name hasPrefix:@"uniforms_"])
			{
				Assert(arg.index >= 0 && arg.index < kMaxBuffers);
				if (arg.index >= 0 && arg.index < kMaxBuffers)
				{
					const char * bufferName = [arg.name cStringUsingEncoding:NSASCIIStringEncoding] + 9 /* strlen("uniforms_") */;
					logDebug("found vs uniform buffer: %s @%d", bufferName, arg.index);
					vsInfo.bufferName[arg.index] = bufferName;
					
					vsInfo.initUniforms(arg);
					addUniforms(arg, 'v');
					
					if (strcmp(bufferName, "ShaderUniforms") == 0)
						vsMainUniformBufferIndex = arg.index;
					
					Assert(vsUniformData[arg.index] == nullptr);
					vsUniformData[arg.index] = malloc(arg.bufferDataSize);
					memset(vsUniformData[arg.index], 0, arg.bufferDataSize);
				}
			}
		}
		
		for (MTLArgument * arg in reflection.fragmentArguments)
		{
			if (arg.type == MTLArgumentTypeBuffer && arg.bufferDataType == MTLDataTypeStruct && [arg.name isEqualToString:@"inputs"])
			{
				//logDebug("found inputs");
			}
			else if (arg.type == MTLArgumentTypeTexture)
			{
				//logDebug("found texture");
				addTexture(arg, 'p');
			}
			else if (arg.type == MTLArgumentTypeBuffer && arg.bufferDataType == MTLDataTypeStruct && [arg.name hasPrefix:@"uniforms_"])
			{
				Assert(arg.index >= 0 && arg.index < kMaxBuffers);
				if (arg.index >= 0 && arg.index < kMaxBuffers)
				{
					const char * bufferName = [arg.name cStringUsingEncoding:NSASCIIStringEncoding] + 9 /* strlen("uniforms_") */;
					logDebug("found ps uniform buffer: %s @%d", bufferName, arg.index);
					psInfo.bufferName[arg.index] = bufferName;
					
					psInfo.initUniforms(arg);
					addUniforms(arg, 'p');
					
					if (strcmp(bufferName, "ShaderUniforms") == 0)
						psMainUniformBufferIndex = arg.index;
					
					Assert(psUniformData[arg.index] == nullptr);
					psUniformData[arg.index] = malloc(arg.bufferDataSize);
					memset(psUniformData[arg.index], 0, arg.bufferDataSize);
				}
			}
		}
		
		initParamIndicesFromUniforms();
	}
}

void ShaderCacheElem_Metal::shut()
{
	errorMessages.clear();
	includedFiles.clear();
	
	//
	
	for (auto & pipeline : m_pipelines)
	{
		pipeline.second = nullptr;
	}
	
	m_pipelines.clear();
	
	//
	
	for (auto & vsTexture : vsTextures)
		vsTexture = nullptr;
	for (auto & psTexture : psTextures)
		psTexture = nullptr;
	
	//
	
	memset(params, -1, sizeof(params));
	
	textureInfos.clear();
	
	for (int i = 0; i < kMaxBuffers; ++i)
	{
		free(vsUniformData[i]);
		free(psUniformData[i]);
		vsUniformData[i] = nullptr;
		psUniformData[i] = nullptr;
	}
	
	uniformInfos.clear();
	
	vsInfo = StageInfo();
	psInfo = StageInfo();
	
	vsFunction = nullptr;
	psFunction = nullptr;
	
	name.clear();
}

void ShaderCacheElem_Metal::load(const char * in_name, const char * in_filenameVs, const char * in_filenamePs, const char * in_outputs)
{
	ScopedLoadTimer loadTimer(in_name);

	shut();
	
	bool success = true;
	
	do
	{
		@autoreleasepool
		{
			__unsafe_unretained id <MTLDevice> device = metal_get_device();
			
			NSError * error = nullptr;
			
			std::string shaderVs_opengl;
			std::string shaderPs_opengl;
			
			success &= preprocessShaderFromFile(in_filenameVs, shaderVs_opengl, 0, errorMessages, includedFiles);
			success &= preprocessShaderFromFile(in_filenamePs, shaderPs_opengl, 0, errorMessages, includedFiles);
			
			if (success == false)
				break;
			
			if (shaderVs_opengl.empty())
			{
				logError("vs is empty. filename=%s", in_filenameVs);
				success &= false;
			}
			if (shaderPs_opengl.empty())
			{
				logError("ps is empty. filename=%s", in_filenamePs);
				success &= false;
			}
			
			if (success == false)
				break;
			
			std::string shaderVs;
			std::string shaderPs;
			
			success &= buildMetalText(shaderVs_opengl.c_str(), 'v', in_outputs, shaderVs);
			success &= buildMetalText(shaderPs_opengl.c_str(), 'p', in_outputs, shaderPs);
			
			if (success == false)
				break;
			
			id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:shaderVs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_vs == nullptr && error != nullptr)
			{
				errorMessages.push_back([[error localizedDescription] cStringUsingEncoding:NSASCIIStringEncoding]);
				logDebug("vs text:\n%s", shaderVs.c_str());
				logError("failed to compile vertex function:");
				NSLog(@"%@", error);
				success = false;
			}
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:shaderPs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
			{
				errorMessages.push_back([[error localizedDescription] cStringUsingEncoding:NSASCIIStringEncoding]);
				logDebug("ps text:\n%s", shaderPs.c_str());
				logError("failed to compile pixel function:");
				NSLog(@"%@", error);
				success = false;
			}
			
			vsFunction = [library_vs newFunctionWithName:@"shader_main"];
			psFunction = [library_ps newFunctionWithName:@"shader_main"];
			
			MTLRenderPipelineReflection * reflection = nullptr;
			
			if (vsFunction != nullptr && psFunction != nullptr)
			{
				// get reflection info for this shader
				
				MTLRenderPipelineDescriptor * pipelineDescriptor = [MTLRenderPipelineDescriptor new];
				pipelineDescriptor.label = @"reflection pipeline";
				pipelineDescriptor.vertexFunction = vsFunction;
				pipelineDescriptor.fragmentFunction = psFunction;
				pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
				
				MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
				
				// we don't know the vertex attributes that will be expected to be set by the shader
				// yet, so we just take a guess and write some large number of them here
				// note : before we used the current vs input bindings, but this is problemetic
				//        if the currently set vs inputs don't match up with the ones being used
				//        during draw. since usually the GX inputs will be bound at this point,
				//        only the GX inputs (position, color, normal, texcoords) will be set,
				//        but the shader may expect some (wildly) different set of attributes
				for ( int i = 0; i < 16; ++i )
				{
					vertexDescriptor.attributes[i].format = MTLVertexFormatFloat;
					vertexDescriptor.attributes[i].bufferIndex = 0;
					vertexDescriptor.attributes[i].offset = i * 4;
				}
				
				vertexDescriptor.layouts[0].stride = 16 * 4;
				vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
				
				pipelineDescriptor.vertexDescriptor = vertexDescriptor;
				
				const MTLPipelineOption pipelineOptions = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
				
				id <MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor options:pipelineOptions reflection:&reflection error:&error];
				
				if (pipelineState == nullptr && error != nullptr)
				{
					errorMessages.push_back([[error localizedDescription] cStringUsingEncoding:NSASCIIStringEncoding]);
					logError("failed to create pipeline state:");
					NSLog(@"%@", error);
				}
				
				pipelineState = nil;
			}
			
			//
			
			if (success)
			{
				init(reflection);
			}
			
			//NSLog(@"reflection retain count: %d\n", (int)reflection.retainCount);
		}
	}
	while (false);
	
	name = in_name;
	vs = in_filenameVs;
	ps = in_filenamePs;
	outputs = in_outputs;

	version++;
}

void ShaderCacheElem_Metal::reload()
{
	const std::string oldName = name;
	const std::string oldVs = vs;
	const std::string oldPs = ps;
	const std::string oldOutputs = outputs;

	load(oldName.c_str(), oldVs.c_str(), oldPs.c_str(), oldOutputs.c_str());
}

bool ShaderCacheElem_Metal::hasIncludedFile(const char * filename) const
{
	for (auto & includedFile : includedFiles)
		if (filename == includedFile)
			return true;
	
	return false;
}

void ShaderCacheElem_Metal::addUniforms(MTLArgument * arg, const char type)
{
	if (!arg.active)
		return;
	
	for (MTLStructMember * uniform in arg.bufferStructType.members)
	{
		const char * name = [uniform.name cStringUsingEncoding:NSASCIIStringEncoding];
		
		// determine the element type, number of elements, and the array length
		
		int elemType = 0;
		int numElems = 0;
		int arrayLen = 1;
		
		switch (uniform.dataType)
		{
		case MTLDataTypeFloat:
			elemType = 'f';
			numElems = 1;
			break;
		case MTLDataTypeFloat2:
			elemType = 'f';
			numElems = 2;
			break;
		case MTLDataTypeFloat3:
			elemType = 'f';
			numElems = 3;
			break;
		case MTLDataTypeFloat4:
			elemType = 'f';
			numElems = 4;
			break;
		case MTLDataTypeFloat4x4:
			elemType = 'm';
			numElems = 16;
			break;
		case MTLDataTypeArray:
			//logDebug("found MTLDataTypeArray. elementType=%d", uniform.arrayType.elementType);
			switch (uniform.arrayType.elementType)
			{
			case MTLDataTypeFloat:
				elemType = 'F';
				numElems = 1;
				arrayLen = uniform.arrayType.arrayLength;
				break;
			case MTLDataTypeFloat2:
				elemType = 'F';
				numElems = 2;
				arrayLen = uniform.arrayType.arrayLength;
				break;
			case MTLDataTypeFloat3:
				elemType = 'F';
				numElems = 3;
				arrayLen = uniform.arrayType.arrayLength;
				break;
			case MTLDataTypeFloat4:
				elemType = 'F';
				numElems = 4;
				arrayLen = uniform.arrayType.arrayLength;
				break;
			case MTLDataTypeFloat4x4:
				elemType = 'M';
				numElems = 16;
				arrayLen = uniform.arrayType.arrayLength;
				break;
			default:
				AssertMsg(false, "unknown MTLDataType: %d", uniform.arrayType.elementType);
				break;
			}
			break;
		default:
			AssertMsg(false, "unknown MTLDataType");
			break;
		}
		
		UniformInfo * found = nullptr;
		
		for (UniformInfo & uniformInfo : uniformInfos)
		{
			if (uniformInfo.name == name)
			{
				found = &uniformInfo;
				break;
			}
		}
		
		Assert(elemType != 0);
		if (elemType != 0)
		{
			UniformInfo * uniformInfo;
			
			if (found == nullptr)
			{
				uniformInfos.resize(uniformInfos.size() + 1);
				
				uniformInfo = &uniformInfos.back();
				uniformInfo->name = name;
			}
			else
			{
				uniformInfo = found;
			}
			
			// when we've already seen this uniform for a different shader stage,
			// we need to assert the types in the vs and ps stages match. this is
			// because the GX api assumes that uniforms are global to all
			// shader stages, and share the same type and semantics across them

			Assert(uniformInfo->elemType == 0 || uniformInfo->elemType == elemType);
			Assert(uniformInfo->numElems == 0 || uniformInfo->numElems == numElems);
			Assert(uniformInfo->arrayLen == 0 || uniformInfo->arrayLen == arrayLen);
			
			// fill in the details (for vs or ps stage)
			
			if (type == 'v')
			{
				uniformInfo->vsBuffer = arg.index;
				uniformInfo->vsOffset = uniform.offset;
			}
			else
			{
				uniformInfo->psBuffer = arg.index;
				uniformInfo->psOffset = uniform.offset;
			}
	
			// fill in the details (shared)

			uniformInfo->elemType = elemType;
			uniformInfo->numElems = numElems;
			uniformInfo->arrayLen = arrayLen;
		}
	}
}

void ShaderCacheElem_Metal::initParamIndicesFromUniforms()
{
	for (int i = 0; i < uniformInfos.size(); ++i)
	{
		auto & uniform = uniformInfos[i];
		
	#define CASE(param, string) if (uniform.name == string) { params[param].set(i); continue; }
		{
			CASE(kSp_ModelViewMatrix, "ModelViewMatrix");
			CASE(kSp_ModelViewProjectionMatrix, "ModelViewProjectionMatrix");
			CASE(kSp_ProjectionMatrix, "ProjectionMatrix");
			CASE(kSp_Texture, "source");
			CASE(kSp_Params, "params");
			CASE(kSp_ShadingParams, "shadingParams");
			CASE(kSp_GradientInfo, "gradientInfo");
			CASE(kSp_GradientMatrix, "gmat");
			CASE(kSp_TextureMatrix, "tmat");
		}
	#undef CASE
	}
}

void ShaderCacheElem_Metal::addTexture(MTLArgument * arg, const char type)
{
	Assert(arg.type == MTLArgumentTypeTexture);
	
	textureInfos.resize(textureInfos.size() + 1);
	
	TextureInfo & info = textureInfos.back();
	info.name = [arg.name cStringUsingEncoding:NSASCIIStringEncoding];
	
	if (type == 'v')
		info.vsOffset = arg.index;
	else
		info.psOffset = arg.index;
}

void ShaderCacheElem_Metal::addTextures(MTLArgument * arg, const char type)
{
	for (MTLStructMember * member in arg.bufferStructType.members)
	{
		if (member.dataType == MTLDataTypeTexture)
		{
			textureInfos.resize(textureInfos.size() + 1);
			
			TextureInfo & info = textureInfos.back();
			info.name = [member.name cStringUsingEncoding:NSASCIIStringEncoding];
			
			if (type == 'v')
				info.vsOffset = member.offset;
			else
				info.psOffset = member.offset;
		}
	}
}

//

void ShaderCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		delete i->second;
		i->second = nullptr;
	}
	
	m_map.clear();
}

void ShaderCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second->reload();
	}
}

void ShaderCache::handleFileChange(const std::string & filename, const std::string & extension)
{
	if (extension != "ps" &&
		extension != "vs" &&
		extension != "txt" &&
		extension != "inc")
	{
		return;
	}
	
	for (auto & shaderCacheItr : m_map)
	{
		ShaderCacheElem_Metal * cacheElem = shaderCacheItr.second;
		
		if (filename == cacheElem->vs ||
			filename == cacheElem->ps ||
			cacheElem->hasIncludedFile(filename.c_str()))
		{
			cacheElem->reload();
			
			if (globals.shader != nullptr && globals.shader->getType() == SHADER_VSPS)
			{
				Shader * shader = static_cast<Shader*>(globals.shader);
				
				if (&shader->getCacheElem() == cacheElem)
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
	
	auto i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return *i->second;
	}
	else
	{
		ShaderCacheElem_Metal * elem = new ShaderCacheElem_Metal();
		
		elem->load(name, filenameVs, filenamePs, outputs);
		
		m_map[key] = elem;
		
		return *elem;
	}
}

//

static ShaderCacheElem_Metal::UniformInfo & getUniformInfo(ShaderCacheElem_Metal & cacheElem, const int index, const int type, const int numElems, const int arrayLen)
{
	Assert(index >= 0 && index < cacheElem.uniformInfos.size());
	auto & info = cacheElem.uniformInfos[index];
	Assert(info.elemType == type && info.numElems == numElems && (islower(type) || info.arrayLen >= arrayLen));
	return info;
}

template <typename T>
T * getVsUniformPtr(ShaderCacheElem_Metal & cacheElem, const int buffer, const int offset)
{
	Assert(cacheElem.vsUniformData[buffer] != nullptr);
	return (T*)(((uint8_t*)cacheElem.vsUniformData[buffer]) + offset);
}

template <typename T>
T * getPsUniformPtr(ShaderCacheElem_Metal & cacheElem, const int buffer, const int offset)
{
	Assert(cacheElem.psUniformData[buffer] != nullptr);
	return (T*)(((uint8_t*)cacheElem.psUniformData[buffer]) + offset);
}

Shader::Shader()
{
}

Shader::Shader(const char * name, const char * outputs)
{
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
	
	m_cacheElem = static_cast<ShaderCacheElem_Metal*>(&g_shaderCache.findOrCreate(name, filenameVs, filenamePs, outputs));
}

void Shader::reload()
{
	m_cacheElem->reload();
}

bool Shader::isValid() const
{
	return
		m_cacheElem->vsFunction != nullptr &&
		m_cacheElem->psFunction != nullptr;
}

int Shader::getVersion() const
{
	return m_cacheElem->version;
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
	for (size_t i = 0; i < m_cacheElem->uniformInfos.size(); ++i)
		if (m_cacheElem->uniformInfos[i].name == name)
			return i;
	return -1;
}

void Shader::getImmediateValuef(const GxImmediateIndex index, float * value) const
{
	if (index >= 0 && index < m_cacheElem->uniformInfos.size())
	{
		auto & info = m_cacheElem->uniformInfos[index];
		
		Assert(info.elemType == 'f');
		if (info.elemType == 'f')
		{
			if (info.vsOffset != -1)
			{
				float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
				memcpy(value, dst, info.numElems * sizeof(float));
			}
			
			if (info.psOffset != -1)
			{
				float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
				memcpy(value, dst, info.numElems * sizeof(float));
			}
		}
	}
}

std::vector<GxImmediateInfo> Shader::getImmediateInfos() const
{
	std::vector<GxImmediateInfo> result;
	result.resize(m_cacheElem->uniformInfos.size());
	
	int count = 0;
	
	for (size_t i = 0; i < m_cacheElem->uniformInfos.size(); ++i)
	{
		auto & u = m_cacheElem->uniformInfos[i];
		auto & su = result[count];
		
		su.name = u.name;
		su.index = i;
		su.type = (GX_IMMEDIATE_TYPE)-1;
		
		if (u.elemType == 'f')
		{
			su.type =
				u.numElems == 1 ? GX_IMMEDIATE_FLOAT :
				u.numElems == 2 ? GX_IMMEDIATE_VEC2 :
				u.numElems == 3 ? GX_IMMEDIATE_VEC3 :
				u.numElems == 4 ? GX_IMMEDIATE_VEC4 :
				(GX_IMMEDIATE_TYPE)-1;
		}
		
		if (su.type == (GX_IMMEDIATE_TYPE)-1)
			continue;
		
		count++;
	}
	
	result.resize(count);
	
	return result;
}

std::vector<GxTextureInfo> Shader::getTextureInfos() const
{
	std::vector<GxTextureInfo> result;
	result.resize(m_cacheElem->textureInfos.size());
	
	int count = 0;
	
	for (size_t i = 0; i < m_cacheElem->textureInfos.size(); ++i)
	{
		auto & t = m_cacheElem->textureInfos[i];
		auto & st = result[count];
		
		st.name = t.name;
		st.index = i;
		st.type = (GX_IMMEDIATE_TYPE)-1;
		
		if (true)
		{
			st.type = GX_IMMEDIATE_TEXTURE_2D;
		}
		
		if (st.type == (GX_IMMEDIATE_TYPE)-1)
			continue;
		
		count++;
	}
	
	result.resize(count);
	
	return result;
}

void Shader::setImmediate(const char * name, float x)
{
	const GxImmediateIndex index = getImmediateIndex(name);
	
	setImmediate(index, x);
}

void Shader::setImmediate(const char * name, float x, float y)
{
	const GxImmediateIndex index = getImmediateIndex(name);
	
	setImmediate(index, x, y);
}

void Shader::setImmediate(const char * name, float x, float y, float z)
{
	const GxImmediateIndex index = getImmediateIndex(name);
	
	setImmediate(index, x, y, z);
}

void Shader::setImmediate(const char * name, float x, float y, float z, float w)
{
	const GxImmediateIndex index = getImmediateIndex(name);
	
	setImmediate(index, x, y, z, w);
}

void Shader::setImmediate(GxImmediateIndex index, float x)
{
	// note : for Metal we don't really care if the current shader is this shader,
	//        but the OpenGL backend does care. so we assert here, to catch
	//        issues that would aride when using the OpenGL backend
	Assert(globals.shader == this);
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 1, 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
			dst[0] = x;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
			dst[0] = x;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 2, 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
			dst[0] = x;
			dst[1] = y;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
			dst[0] = x;
			dst[1] = y;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 3, 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
		}
	}
}

void Shader::setImmediate(GxImmediateIndex index, float x, float y, float z, float w)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'f', 4, 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
			dst[0] = x;
			dst[1] = y;
			dst[2] = z;
			dst[3] = w;
		}
	}
}

void Shader::setImmediateMatrix4x4(const char * name, const float * matrix)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	const GxImmediateIndex index = getImmediateIndex(name);
	
	setImmediateMatrix4x4(index, matrix);
}

void Shader::setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'm', 16, 1);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
			memcpy(dst, matrix, 16 * sizeof(float));
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
			memcpy(dst, matrix, 16 * sizeof(float));
		}
	}
}

void Shader::setImmediateMatrix4x4Array(const char * name, const float * matrices, const int numMatrices)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	const GxImmediateIndex index = getImmediateIndex(name);
	
	setImmediateMatrix4x4Array(index, matrices, numMatrices);
}

void Shader::setImmediateMatrix4x4Array(GxImmediateIndex index, const float * matrices, const int numMatrices)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	if (index >= 0)
	{
		auto & info = getUniformInfo(*m_cacheElem, index, 'M', 16, numMatrices);
		
		if (info.vsOffset != -1)
		{
			float * dst = getVsUniformPtr<float>(*m_cacheElem, info.vsBuffer, info.vsOffset);
			memcpy(dst, matrices, 16 * sizeof(float) * numMatrices);
		}
		
		if (info.psOffset != -1)
		{
			float * dst = getPsUniformPtr<float>(*m_cacheElem, info.psBuffer, info.psOffset);
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

static void setTextureSamplerUniform(ShaderCacheElem_Metal * cacheElem, GxImmediateIndex index, const bool filter, const bool clamp)
{
	Assert(index >= 0 && index < cacheElem->textureInfos.size());
	auto & info = cacheElem->textureInfos[index];
	
	const int sampler_index = ((filter ? 2 : 0) << 1) | clamp;
	
	if (info.vsOffset >= 0 && info.vsOffset < ShaderCacheElem_Metal::kMaxVsTextures)
		cacheElem->vsTextureSamplers[info.vsOffset] = sampler_index;
	if (info.psOffset >= 0 && info.psOffset < ShaderCacheElem_Metal::kMaxPsTextures)
		cacheElem->psTextureSamplers[info.psOffset] = sampler_index;
}


static void setTextureUniform(ShaderCacheElem_Metal * cacheElem, GxImmediateIndex index, int unit, GxTextureId texture)
{
	Assert(index >= 0 && index < cacheElem->textureInfos.size());
	auto & info = cacheElem->textureInfos[index];
	
	if (texture == 0)
	{
		if (info.vsOffset >= 0 && info.vsOffset < ShaderCacheElem_Metal::kMaxVsTextures)
			cacheElem->vsTextures[info.vsOffset] = nullptr;
		if (info.psOffset >= 0 && info.psOffset < ShaderCacheElem_Metal::kMaxPsTextures)
			cacheElem->psTextures[info.psOffset] = nullptr;
	}
	else
	{
		auto i = s_textures.find(texture);
		Assert(i != s_textures.end());

		if (i != s_textures.end())
		{
			auto metal_texture = i->second;
			
			if (info.vsOffset >= 0 && info.vsOffset < ShaderCacheElem_Metal::kMaxVsTextures)
				cacheElem->vsTextures[info.vsOffset] = metal_texture;
			if (info.psOffset >= 0 && info.psOffset < ShaderCacheElem_Metal::kMaxPsTextures)
				cacheElem->psTextures[info.psOffset] = metal_texture;
		}
	}
}

void Shader::setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	const int index = getTextureIndex(*m_cacheElem, name);
	
	if (index >= 0)
	{
		setTextureUniform(m_cacheElem, index, unit, texture);
		setTextureSamplerUniform(m_cacheElem, index, filtered, clamp);
	}
}

void Shader::setTexture(GxImmediateIndex index, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	setTextureUniform(m_cacheElem, index, unit, texture);
	setTextureSamplerUniform(m_cacheElem, index, filtered, clamp);
}

void Shader::setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	not_implemented;
}

void Shader::setTextureCube(const char * name, int unit, GxTextureId texture, bool filtered)
{
	not_implemented;
}

void Shader::setTexture3d(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp)
{
	const int index = getTextureIndex(*m_cacheElem, name);
	
	if (index >= 0)
	{
		setTextureUniform(m_cacheElem, index, unit, texture);
		setTextureSamplerUniform(m_cacheElem, index, filtered, clamp);
	}
}

void Shader::setBuffer(const char * name, const ShaderBuffer & buffer)
{
	Assert(globals.shader == this); // see comment for setImmediate(index, x) for why this exists
	
	buffer.markMetalBufferIsUsed();
	
	__unsafe_unretained id <MTLBuffer> metal_buffer = (__bridge id <MTLBuffer>)buffer.getMetalBuffer();
	
	const int vsIndex = m_cacheElem->vsInfo.getBufferIndex(name);
	const int psIndex = m_cacheElem->psInfo.getBufferIndex(name);
	
	if (vsIndex >= 0)
	{
		AssertMsg(
			buffer.getBufferSize() <= m_cacheElem->vsInfo.uniformBufferSize[vsIndex],
			"the buffer is larger than what the vs expects. length=%d, expected=%d",
			buffer.getBufferSize(),
			m_cacheElem->vsInfo.uniformBufferSize[vsIndex]);
		
		m_cacheElem->vsBuffers[vsIndex] = metal_buffer;
	}
	
	if (psIndex >= 0)
	{
		AssertMsg(
			buffer.getBufferSize() <= m_cacheElem->psInfo.uniformBufferSize[psIndex],
			"the buffer is larger than what the ps expects. length=%d, expected=%d",
			buffer.getBufferSize(),
			m_cacheElem->psInfo.uniformBufferSize[psIndex]);
		
		m_cacheElem->psBuffers[psIndex] = metal_buffer;
	}
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
