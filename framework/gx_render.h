#pragma once

#include "framework.h"

class ColorTargetProperties
{
public:
	struct
	{
		int width = 0;
		int height = 0;
	} dimensions;
	SURFACE_FORMAT format = SURFACE_RGBA8;
	Color clearColor = colorBlack;

	void init(const int in_width, const int in_height, SURFACE_FORMAT in_format, const Color in_clearColor)
	{
		dimensions.width = in_width;
		dimensions.height = in_height;
		format = in_format;
		clearColor = in_clearColor;
	}
};

class ColorTargetBase
{
public:
	virtual ~ColorTargetBase() { }
	
	virtual bool init(const int width, const int height, SURFACE_FORMAT format, const Color & clearColor) = 0;
	virtual bool init(const ColorTargetProperties & properties) = 0;
	
	virtual void setClearColor(const float r, const float g, const float b, const float a) = 0;
	virtual const Color & getClearColor() const = 0;
	
	virtual GxTextureId getTextureId() const = 0;
};

//

class DepthTargetProperties
{
public:
	struct
	{
		int width = 0;
		int height = 0;
	} dimensions;
	DEPTH_FORMAT format = DEPTH_FLOAT32;
	float clearDepth = 1.f;

	void init(const int in_width, const int in_height, DEPTH_FORMAT in_format, const float in_clearDepth)
	{
		dimensions.width = in_width;
		dimensions.height = in_height;
		format = in_format;
		clearDepth = in_clearDepth;
	}
};

class DepthTargetBase
{
public:
	virtual ~DepthTargetBase() { }
	
	virtual bool init(const int width, const int height, DEPTH_FORMAT format, const float clearDepth) = 0;
	virtual bool init(const DepthTargetProperties & properties) = 0;
	
	virtual void setClearDepth(const float depth) = 0;
	virtual float getClearDepth() const = 0;
	
	virtual GxTextureId getTextureId() const = 0;
};

void pushRenderPass(ColorTarget * target, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName);
void pushRenderPass(ColorTarget ** targets, const int numTargets, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName);
void popRenderPass();

#if ENABLE_METAL
	#include "gx-metal/renderTarget.h"
#endif
