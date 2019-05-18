#pragma once

#import "framework.h"

#if ENABLE_METAL

#import <string>
#import <vector>

struct ShaderCacheElem;

#ifdef __OBJC__

#import <map>
#import <Metal/Metal.h>

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

	struct StageInfo
	{
		struct
		{
			int offset = -1;

			void set(const int offset)
			{
				this->offset = offset;
			}
		} params[kSp_MAX];
		
		int uniformBufferIndex = -1;
		int uniformBufferSize = 0;
		
		void init(MTLArgument * arg)
		{
			uniformBufferIndex = arg.index;
			uniformBufferSize = arg.bufferDataSize;
		
			for (MTLStructMember * uniform in arg.bufferStructType.members)
			{
			#define CASE(param, string) if ([uniform.name isEqualToString:@string]) { params[param].set(uniform.offset); }
				{
					CASE(kSp_ModelViewMatrix, "ModelViewMatrix");
					CASE(kSp_ModelViewProjectionMatrix, "ModelViewProjectionMatrix");
					CASE(kSp_ProjectionMatrix, "ProjectionMatrix");
					CASE(kSp_SkinningMatrices, "skinningMatrices");
					CASE(kSp_Texture, "texture0");
					CASE(kSp_Params, "params");
					CASE(kSp_ShadingParams, "shadingParams");
					CASE(kSp_GradientInfo, "gradientInfo");
					CASE(kSp_GradientMatrix, "gmat");
					CASE(kSp_TextureMatrix, "tmat");
				}
			#undef CASE
			}
		}
	};

	struct UniformInfo
	{
		std::string name;
		int vsOffset = -1;
		int psOffset = -1;
		int elemType = -1;
		int numElems = 0;
	};
	
	void * vs = nullptr;
	void * ps = nullptr;
	
	StageInfo vsInfo;
	StageInfo psInfo;

	std::vector<UniformInfo> uniformInfos;
	
	void * vsUniformData = nullptr;
	void * psUniformData = nullptr;
	
	~ShaderCacheElem()
	{
		shut();
	}
	
	void init(MTLRenderPipelineReflection * reflection)
	{
	/*
		// todo : VS_POSITION etc
		glBindAttribLocation(program, VS_POSITION,      "in_position4");
		glBindAttribLocation(program, VS_NORMAL,        "in_normal");
		glBindAttribLocation(program, VS_COLOR,         "in_color");
		glBindAttribLocation(program, VS_TEXCOORD0,     "in_texcoord0");
		glBindAttribLocation(program, VS_TEXCOORD1,     "in_texcoord1");
		glBindAttribLocation(program, VS_BLEND_INDICES, "in_skinningBlendIndices");
		glBindAttribLocation(program, VS_BLEND_WEIGHTS, "in_skinningBlendWeights");
		checkErrorGL();
	*/

		// cache uniform offsets
		
		if (reflection != nullptr)
		{
			for (MTLArgument * arg in reflection.vertexArguments)
			{
				if (arg.bufferDataSize == MTLDataTypeStruct && [arg.name isEqualToString:@"inputs"])
				{
					NSLog(@"found inputs");
				}
				else if (arg.bufferDataType == MTLDataTypeStruct && [arg.name isEqualToString:@"uniforms"])
				{
					vsInfo.init(arg);
					addUniforms(arg, 'v');
					//Assert(vsUniformData == nullptr); // todo : enable assert
					vsUniformData = malloc(arg.bufferDataSize);
					break;
				}
			}
			
			for (MTLArgument * arg in reflection.fragmentArguments)
			{
				if (arg.bufferDataType == MTLDataTypeStruct && [arg.name isEqualToString:@"uniforms"])
				{
					psInfo.init(arg);
					addUniforms(arg, 'p');
					//Assert(psUniformData == nullptr); // todo : enable assert
					psUniformData = malloc(arg.bufferDataSize);
					break;
				}
			}
		}
	}
	
	void shut()
	{
		free(vsUniformData);
		free(psUniformData);
		
		vsUniformData = nullptr;
		psUniformData = nullptr;
	}
	
	void addUniforms(MTLArgument * arg, const char type)
	{
		for (MTLStructMember * uniform in arg.bufferStructType.members)
		{
			const char * name = [uniform.name cStringUsingEncoding:NSASCIIStringEncoding];
			
			bool found = false;
			
			for (UniformInfo & uniformInfo : uniformInfos)
			{
				if (uniformInfo.name == name)
				{
					found = true;
					
					if (type == 'v')
						uniformInfo.vsOffset = uniform.offset;
					else
						uniformInfo.psOffset = uniform.offset;
				}
			}
			
			if (found == false)
			{
				uniformInfos.resize(uniformInfos.size() + 1);
				
				UniformInfo & uniformInfo = uniformInfos.back();
				uniformInfo.name = name;
				
				if (type == 'v')
					uniformInfo.vsOffset = uniform.offset;
				else
					uniformInfo.psOffset = uniform.offset;
				
				switch (uniform.dataType)
				{
				case MTLDataTypeFloat:
					uniformInfo.elemType = 'f';
					uniformInfo.numElems = 1;
					break;
				case MTLDataTypeFloat2:
					uniformInfo.elemType = 'f';
					uniformInfo.numElems = 2;
					break;
				case MTLDataTypeFloat3:
					uniformInfo.elemType = 'f';
					uniformInfo.numElems = 3;
					break;
				case MTLDataTypeFloat4:
					uniformInfo.elemType = 'f';
					uniformInfo.numElems = 4;
					break;
				case MTLDataTypeFloat4x4:
					uniformInfo.elemType = 'm';
					uniformInfo.numElems = 16;
					break;
				default:
					//Assert(false); // todo : enable assert
					break;
				}
			}
		}
	}
	
	mutable std::map<uint32_t, id <MTLRenderPipelineState>> m_pipelines;
	
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

class ShaderCache
{
	ShaderCacheElem * m_cacheElem = nullptr; // todo : make private
	
public:
	ShaderCacheElem & findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs);
	
	void clear() { }
	void reload() { }
};

//

class Shader : public ShaderBase
{
public:
	ShaderCacheElem * m_cacheElem = nullptr; // todo : make private
	
	Shader();
	Shader(const char * name, const char * outputs = nullptr);
	Shader(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs = nullptr);
	virtual ~Shader();
	
	void load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs = nullptr);
	virtual bool isValid() const override { return true; } // todo
	virtual GxShaderId getProgram() const override { return 0; }; // todo : make internally accessible only and add functionality on a per use-case basis
	virtual SHADER_TYPE getType() const override { return SHADER_VSPS; }
	virtual int getVersion() const override { return 1; } // todo
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const override { return false; } // todo

	GxImmediateIndex getImmediate(const char * name);
	
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

	const ShaderCacheElem & getCacheElem() const { return *m_cacheElem; }
};

extern ShaderCache g_shaderCache;

#endif
