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

#pragma once

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#elif defined(ANDROID)
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

#include "framework.h"

#if ENABLE_OPENGL

#include <map>
#include <string>
#include <vector>

class ShaderCacheElem
{
public:
	enum ShaderParam
	{
		kSp_ModelViewMatrix,
		kSp_ModelViewProjectionMatrix,
		kSp_ProjectionMatrix,
		kSp_Texture,
		kSp_Params,
		kSp_ShadingParams,
		kSp_GradientInfo,
		kSp_GradientMatrix,
		kSp_TextureMatrix,
		kSp_MAX
	};

	std::string name;
	std::string vs;
	std::string ps;
	std::string outputs;
	
	GLuint program;
	
	int version;
	std::vector<std::string> errorMessages;
	std::vector<std::string> includedFiles;

	struct
	{
		GLint index;

		void set(GLint index)
		{
			this->index = index;
		}
	} params[kSp_MAX];

	ShaderCacheElem();
	void free();
	void load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs);
	void reload();
	
	bool hasIncludedFile(const char * filename) const;
};

class ShaderCache : public ResourceCacheBase
{
public:
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
	
	typedef std::map<Key, ShaderCacheElem> Map;
	
	Map m_map;
	
	virtual void clear() override;
	virtual void reload() override;
	virtual void handleFileChange(const std::string & filename, const std::string & extension) override;
	ShaderCacheElem & findOrCreate(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs);
};

#if ENABLE_COMPUTE_SHADER

class ComputeShaderCacheElem
{
public:
	std::string name;
	int groupSx;
	int groupSy;
	int groupSz;

	GLuint program;
	
	int version;
	std::vector<std::string> errorMessages;
	std::vector<std::string> includedFiles;
	
	ComputeShaderCacheElem();
	void free();
	void load(const char * filename, const int groupSx, const int groupSy, const int groupSz);
	void reload();
	
	bool hasIncludedFile(const char * filename) const;
};

class ComputeShaderCache : public ResourceCacheBase
{
public:
	typedef std::map<std::string, ComputeShaderCacheElem> Map;

	Map m_map;

	virtual void clear() override;
	virtual void reload() override;
	virtual void handleFileChange(const std::string & filename, const std::string & extension) override;
	ComputeShaderCacheElem & findOrCreate(const char * filename, const int groupSx, const int groupSy, const int groupSz);
};

#endif

#endif
