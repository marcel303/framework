#pragma once

#include "framework.h"
#include "gx_render.h"
#include "metal.h"

class ColorTarget : ColorTargetBase
{
	void * m_colorTexture = nullptr;
	int m_colorTextureId = 0;
	bool m_ownsTexture = true;
	
	ColorTargetProperties properties;

public:
	ColorTarget()
	{
	}
	
	ColorTarget(void * colorTexture)
	{
		m_colorTexture = colorTexture;
		m_ownsTexture = false;
	}
	
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
	
	virtual GxTextureId getTextureId() const override final;
	
	virtual int getWidth() const override final
	{
		return properties.dimensions.width;
	}
	
	virtual int getHeight() const override final
	{
		return properties.dimensions.height;
	}
	
	void * getMetalTexture() const { return m_colorTexture; }
};

//

class DepthTarget : DepthTargetBase
{
	void * m_depthTexture = nullptr;
	int m_depthTextureId = 0;
	bool m_ownsTexture = true;
	
	DepthTargetProperties properties;
	
public:
	DepthTarget()
	{
	}
	
	DepthTarget(void * depthTexture)
	{
		m_depthTexture = depthTexture;
		m_ownsTexture = false;
	}
	
	virtual ~DepthTarget() override final;
	
	virtual bool init(const int width, const int height, DEPTH_FORMAT format, const bool enableTexture, const float clearDepth) override final
	{
		DepthTargetProperties properties;
		properties.init(width, height, format, enableTexture, clearDepth);
		
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
	
	virtual bool isTextureEnabled() const override final
	{
		return properties.enableTexture;
	}
	
	virtual GxTextureId getTextureId() const override final;
	
	virtual int getWidth() const override final
	{
		return properties.dimensions.width;
	}
	
	virtual int getHeight() const override final
	{
		return properties.dimensions.height;
	}
	
	void * getMetalTexture() const { return m_depthTexture; }
};
