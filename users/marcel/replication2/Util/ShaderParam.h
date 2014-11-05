#ifndef SHADERPARAM_H
#define SHADERPARAM_H
#pragma once

#include <map>
#include <string>
#include "Mat4x4.h"
#include "ResBaseTex.h"
#include "Vec3.h"
#include "Vec4.h"

enum SHADER_PARAM_TYPE
{
	SHPARAM_FLOAT,
	SHPARAM_VEC3,
	SHPARAM_VEC4,
	SHPARAM_MAT4X4,
	SHPARAM_TEX
};

enum REGISTER_TYPE
{
	SHREG_CONSTANT,
	SHREG_SAMPLER
};

class ShaderParam
{
public:
	void Setup(REGISTER_TYPE registerType, int registerIndex)
	{
		m_registerType = registerType;
		m_registerIndex = registerIndex;
	}

	void Assign(SHADER_PARAM_TYPE type, const void* v, int floatCount)
	{
		m_type = type;
		memcpy(m_v, v, floatCount * sizeof(float));
	}

	void AssignP(SHADER_PARAM_TYPE type, void* p)
	{
		m_type = type;
		m_p = p;
	}

	inline ShaderParam& operator=(float v)
	{
		Assign(SHPARAM_FLOAT, &v, 1);
		return *this;
	}

	inline ShaderParam& operator=(const Vec3& v)
	{
		Assign(SHPARAM_VEC3, &v, 3);
		return *this;
	}

	inline ShaderParam& operator=(const Vec4& v)
	{
		Assign(SHPARAM_VEC4, &v, 4);
		return *this;
	}

	inline ShaderParam& operator=(const Mat4x4& v)
	{
		Assign(SHPARAM_MAT4X4, &v, 16);
		return *this;
	}

	inline ShaderParam& operator=(ResBaseTex* tex)
	{
		AssignP(SHPARAM_TEX, tex);
		return *this;
	}

	SHADER_PARAM_TYPE m_type;
	REGISTER_TYPE m_registerType;
	int m_registerIndex;
	union
	{
		float m_v[16];
		void* m_p;
	};
};

class ShaderParamList
{
public:
	ShaderParam& operator[](const std::string& name);

//private:
	typedef std::map<std::string, ShaderParam> ParamColl;
	typedef ParamColl::iterator ParamCollItr;

	std::map<std::string, ShaderParam> m_parameters;
};

#endif
