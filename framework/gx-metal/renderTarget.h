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

#include "framework.h"
#include "gx_render.h"
#include "metal.h"

class ColorTarget : ColorTargetBase
{
	void * m_colorTexture = nullptr; // texture for use as a render target. ignores sRGB pixel format, to make it possible to write to it directly using sRGB data, and read it back linearly (using the texture view below)
	
	void * m_colorTextureView = nullptr; // view for use with texture sampling. uses sRGB pixel format when requested
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
		m_colorTextureView = colorTexture;
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
	
	void free();
	
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
	void * getMetalTextureView() const { return m_colorTextureView; }
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
	
	void free();
	
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
