#pragma once

#include "framework.h"
#include "gx_render.h"
#include "metal.h"

class ColorTarget : ColorTargetBase
{
	void * m_colorTexture = nullptr;
	
	ColorTargetProperties properties;
	
	Color clearColor = colorBlack;

public:
	virtual ~ColorTarget() override final;
	
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

//

class DepthTarget : DepthTargetBase
{
	void * m_depthTexture = nullptr;
	
	DepthTargetProperties properties;
	
	float clearDepth = 1.f;
	
public:
	virtual ~DepthTarget() override final;
	
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
