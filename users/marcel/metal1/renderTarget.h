#pragma once

#include "metal.h"

class Color // todo : remove
{
public:
	float r, g, b, a;
};

static const Color colorBlack = { 0, 0, 0, 0 };

class ColorTargetProperties
{
public:
	int width = 0;
	int height = 0;
	SURFACE_FORMAT format = SURFACE_RGBA8;
	Color clearColor = colorBlack;

	void init(const int in_width, const int in_height, SURFACE_FORMAT in_format, const Color in_clearColor)
	{
		width = in_width;
		height = in_height;
		format = in_format;
		clearColor = in_clearColor;
	}
};

class ColorTarget
{
public:
	void * m_colorTexture = nullptr;
	
	ColorTargetProperties properties;
	
	Color clearColor = colorBlack;

//public:
	bool init(const int width, const int height, SURFACE_FORMAT format, const Color & clearColor)
	{
		ColorTargetProperties properties;
		
		properties.init(width, height, format, clearColor);
		
		return init(properties);
	}
	
	bool init(const ColorTargetProperties & properties);
	
	void setClearColor(const float r, const float g, const float b, const float a)
	{
		clearColor.r = r;
		clearColor.g = g;
		clearColor.b = b;
		clearColor.a = a;
	}
	
	void * getMetalTexture() const { return m_colorTexture; }
};

class DepthTargetProperties
{
public:
	int width = 0;
	int height = 0;
	DEPTH_FORMAT format = DEPTH_FLOAT32;
	float clearDepth = 1.f;

	void init(const int in_width, const int in_height, DEPTH_FORMAT in_format, const float in_clearDepth)
	{
		width = in_width;
		height = in_height;
		format = in_format;
		clearDepth = in_clearDepth;
	}
};

class DepthTarget
{
public:
	void * m_depthTexture = nullptr;
	
	DepthTargetProperties properties;
	
	float clearDepth = 1.f;
	
//public:
	bool init(const int width, const int height, DEPTH_FORMAT format, const float clearDepth)
	{
		DepthTargetProperties properties;
		properties.init(width, height, format, clearDepth);
		
		return init(properties);
	}
	
	bool init(const DepthTargetProperties & properties);
	
	void setClearDepth(const float depth)
	{
		clearDepth = depth;
	}
};
