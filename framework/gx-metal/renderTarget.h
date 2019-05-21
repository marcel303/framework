#pragma once

#include "framework.h"
#include "metal.h"

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

class ColorTargetBase
{
	virtual bool init(const int width, const int height, SURFACE_FORMAT format, const Color & clearColor) = 0;
	virtual bool init(const ColorTargetProperties & properties) = 0;
	
	virtual void setClearColor(const float r, const float g, const float b, const float a) = 0;
	virtual const Color & getClearColor() const = 0;
};

class ColorTarget : ColorTargetBase
{
	void * m_colorTexture = nullptr;
	
	ColorTargetProperties properties;
	
	Color clearColor = colorBlack;

public:
	virtual bool init(const int width, const int height, SURFACE_FORMAT format, const Color & clearColor) override final
	{
		ColorTargetProperties properties;
		
		properties.init(width, height, format, clearColor);
		
		return init(properties);
	}
	
	virtual bool init(const ColorTargetProperties & properties) override final;
	
	virtual void setClearColor(const float r, const float g, const float b, const float a) override final
	{
		clearColor.r = r;
		clearColor.g = g;
		clearColor.b = b;
		clearColor.a = a;
	}
	
	virtual const Color & getClearColor() const override final
	{
		return clearColor;
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

class DepthTargetBase
{
public:
	virtual bool init(const int width, const int height, DEPTH_FORMAT format, const float clearDepth) = 0;
	virtual bool init(const DepthTargetProperties & properties) = 0;
	
	virtual void setClearDepth(const float depth) = 0;
	virtual float getClearDepth() const = 0;
};

class DepthTarget : DepthTargetBase
{
	void * m_depthTexture = nullptr;
	
	DepthTargetProperties properties;
	
	float clearDepth = 1.f;
	
public:
	virtual bool init(const int width, const int height, DEPTH_FORMAT format, const float clearDepth) override final
	{
		DepthTargetProperties properties;
		properties.init(width, height, format, clearDepth);
		
		return init(properties);
	}
	
	virtual bool init(const DepthTargetProperties & properties) override final;
	
	virtual void setClearDepth(const float depth) override final
	{
		clearDepth = depth;
	}
	
	virtual float getClearDepth() const override final
	{
		return clearDepth;
	}
	
	void * getMetalTexture() const { return m_depthTexture; }
};
