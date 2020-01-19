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

#pragma once

#import "framework.h"

#if ENABLE_METAL

#import <string>
#import <vector>

struct ShaderCacheElem;
struct ShaderCacheElem_Metal;

struct ShaderCacheElem
{
	enum ShaderParam
	{
		kSp_ModelViewMatrix,
		kSp_ModelViewProjectionMatrix,
		kSp_ProjectionMatrix,
		kSp_SkinningMatrices,
		kSp_Texture,
		kSp_Params,
		kSp_ShadingParams,
		kSp_GradientInfo,
		kSp_GradientMatrix,
		kSp_TextureMatrix,
		kSp_MAX
	};
	
	struct
	{
		int index = -1;

		void set(const int index)
		{
			this->index = index;
		}
	} params[kSp_MAX];
};

#ifdef __OBJC__

#import <map>
#import <Metal/Metal.h>

struct ShaderCacheElem_Metal : ShaderCacheElem
{
	static const int kMaxBuffers = 16;
	
	static const int kMaxVsTextures = 2;
	static const int kMaxPsTextures = 8;
	
	struct StageInfo
	{
		int uniformBufferSize[kMaxBuffers] = { };
		
		void initUniforms(MTLArgument * arg)
		{
			uniformBufferSize[arg.index] = arg.bufferDataSize;
		}
	};

	struct UniformInfo
	{
		std::string name;
		int vsBuffer = -1;
		int vsOffset = -1;
		int psBuffer = -1;
		int psOffset = -1;
		int elemType = -1;
		int numElems = 0;
	};
	
	struct TextureInfo
	{
		std::string name;
		int vsOffset = -1;
		int psOffset = -1;
	};
	
	std::string name;
	std::string vs;
	std::string ps;
	std::string outputs;
	
	int version = 0;
	
	id <MTLFunction> vsFunction = nullptr;
	id <MTLFunction> psFunction = nullptr;
	
	StageInfo vsInfo;
	StageInfo psInfo;

	std::vector<UniformInfo> uniformInfos;
	
	void * vsUniformData[kMaxBuffers] = { };
	void * psUniformData[kMaxBuffers] = { };
	
	std::vector<TextureInfo> textureInfos;
	
	id <MTLTexture> vsTextures[kMaxVsTextures] = { };
	id <MTLTexture> psTextures[kMaxPsTextures] = { };
	
	uint8_t vsTextureSamplers[kMaxPsTextures] = { };
	uint8_t psTextureSamplers[kMaxPsTextures] = { };
	
	mutable std::map<uint32_t, id <MTLRenderPipelineState>> m_pipelines;
	
	~ShaderCacheElem_Metal()
	{
		shut();
	}
	
	void init(MTLRenderPipelineReflection * reflection);
	void shut();
	
	void load(const char * in_name, const char * in_filenameVs, const char * in_filenamePs, const char * in_outputs);
	
	void reload()
	{
		const std::string oldName = name;
		const std::string oldVs = vs;
		const std::string oldPs = ps;
		const std::string oldOutputs = outputs;

		load(oldName.c_str(), oldVs.c_str(), oldPs.c_str(), oldOutputs.c_str());
	}
	
	void addUniforms(MTLArgument * arg, const char type);
	void initParamIndicesFromUniforms();
	
	void addTexture(MTLArgument * arg, const char type);
	void addTextures(MTLArgument * arg, const char type);
	
	id <MTLRenderPipelineState> findPipelineState(const uint32_t hash) const
	{
		return m_pipelines[hash]; // todo : nice iterator lookup etc
	}
	
	void addPipelineState(const uint32_t hash, id <MTLRenderPipelineState> state) const
	{
		m_pipelines[hash] = state;
	}
};

#endif

#include <map>

// todo : make shader cache stuff internal

class ShaderCache
{
	class Key
	{
	public:
		std::string name;
		std::string outputs;
		
		inline bool operator<(const Key & other) const
		{
			if (name != other.name)
				return name < other.name;
			if (outputs != other.outputs)
				return outputs < other.outputs;
			return false;
		}
	};
	
	typedef std::map<Key, ShaderCacheElem_Metal*> Map;
	
	Map m_map;
	
public:
	void clear();
	void reload();
	void handleSourceChanged(const char * name);
	ShaderCacheElem & findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs);
};

//

class Shader : public ShaderBase
{
public:
	ShaderCacheElem_Metal * m_cacheElem = nullptr; // todo : make private
	
	Shader();
	Shader(const char * name, const char * outputs = nullptr);
	Shader(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs = nullptr);
	virtual ~Shader();
	
	void load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs = nullptr);
	virtual bool isValid() const override;
	virtual GxShaderId getProgram() const override { return 0; }; // todo : make internally accessible only and add functionality on a per use-case basis
	virtual SHADER_TYPE getType() const override { return SHADER_VSPS; }
	virtual int getVersion() const override;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const override { return false; } // todo

	GxImmediateIndex getImmediateIndex(const char * name);
	void getImmediateValuef(const GxImmediateIndex index, float * value);
	
	std::vector<GxImmediateInfo> getImmediateInfos() const;
	
	void setImmediate(const char * name, float x);	
	void setImmediate(const char * name, float x, float y);
	void setImmediate(const char * name, float x, float y, float z);
	void setImmediate(const char * name, float x, float y, float z, float w);
	void setImmediate(GxImmediateIndex index, float x);
	void setImmediate(GxImmediateIndex index, float x, float y);
	void setImmediate(GxImmediateIndex index, float x, float y, float z);
	void setImmediate(GxImmediateIndex index, float x, float y, float z, float w);
	void setImmediateMatrix4x4(const char * name, const float * matrix);
	void setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix);
	void setImmediateMatrix4x4Array(GxImmediateIndex index, const float * matrices, const int numMatrices);
	
// todo : texture units do not make much sense ..
	void setTextureUnit(const char * name, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTextureUnit(GxImmediateIndex index, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTexture(const char * name, int unit, GxTextureId texture);
	void setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureUniform(GxImmediateIndex index, int unit, GxTextureId texture);
	void setTextureArray(const char * name, int unit, GxTextureId texture);
	void setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureCube(const char * name, int unit, GxTextureId texture);
	void setBuffer(const char * name, const ShaderBuffer & buffer);
	void setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer);
	void setBufferRw(const char * name, const ShaderBufferRw & buffer);
	void setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer);

	const ShaderCacheElem & getCacheElem() const;
};

extern ShaderCache g_shaderCache;

#endif
