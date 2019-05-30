#pragma once

#include "framework.h"
#include "gx_render.h"

class ColorTarget : ColorTargetBase
{
	GxTextureId m_colorTextureId = 0;
	
	ColorTargetProperties properties;

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
		properties.clearColor.r = r;
		properties.clearColor.g = g;
		properties.clearColor.b = b;
		properties.clearColor.a = a;
	}
	
	virtual const Color & getClearColor() const override final
	{
		return properties.clearColor;
	}
	
	virtual GxTextureId getTextureId() const override final
	{
		return m_colorTextureId;
	}
	
	virtual int getWidth() const override final
	{
		return properties.dimensions.width;
	}
	
	virtual int getHeight() const override final
	{
		return properties.dimensions.height;
	}
};

//

class DepthTarget : DepthTargetBase
{
	GxTextureId m_depthTextureId = 0;
	
	DepthTargetProperties properties;
	
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
		properties.clearDepth = depth;
	}
	
	virtual float getClearDepth() const override final
	{
		return properties.clearDepth;
	}
	
	virtual GxTextureId getTextureId() const override final
	{
		return m_depthTextureId;
	}
	
	virtual int getWidth() const override final
	{
		return properties.dimensions.width;
	}
	
	virtual int getHeight() const override final
	{
		return properties.dimensions.height;
	}
};
