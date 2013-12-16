#pragma once

#include <string>
#include <vector>
#include "Mat4x4.h"
#include "Types.h"
#include "Vec3.h"
#include "Vec4.h"

class IArchive
{
public:
	virtual bool Exists(const char* name, int index = 0);
	virtual void Begin(const char* name, int index = 0);
	virtual void End();

	virtual std::vector<std::string> GetKeys() = 0;

	virtual IArchive& operator[](const std::string& key) = 0;

	// Read.
	#define default _default
	virtual int operator()(const std::string& key, int default) const = 0;
	virtual float operator()(const std::string& key, float default) const = 0;
	virtual std::string operator()(const std::string& key, const std::string& default) const = 0;
	virtual Vec3 operator()(const std::string& key, const Vec3& default) const = 0;
	virtual Vec4 operator()(const std::string& key, const Vec4& default) const = 0;
	virtual Mat4x4 operator()(const std::string& key, const Mat4x4& default) const = 0;
	#undef default

	// Write.
	virtual IArchive& operator=(int v) = 0;
	virtual IArchive& operator=(uint32_t v) = 0;
	virtual IArchive& operator=(float v) = 0;
	virtual IArchive& operator=(const std::string& v) = 0;
	virtual IArchive& operator=(const Vec3& v) = 0;
	virtual IArchive& operator=(const Vec4& v) = 0;
	virtual IArchive& operator=(const Mat4x4& v) = 0;
};
