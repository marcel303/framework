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

#include "framework.h"
#include "vfxNodeSurface.h"

extern int VFXGRAPH_SX;
extern int VFXGRAPH_SY;

extern Surface * g_currentVfxSurface;

VFX_ENUM_TYPE(surfaceFormat)
{
	elem("rgba8");
	elem("rgba16f");
}

VFX_ENUM_TYPE(surfaceViewMode)
{
	elem("screen");
	elem("perspective");
}

VFX_NODE_TYPE(VfxNodeSurface)
{
	typeName = "draw.surface";
	
	in("source", "draw", "", "draw");
	inEnum("format", "surfaceFormat");
	in("width", "int");
	in("height", "int");
	in("clear", "bool", "1");
	in("clearColor", "color");
	in("depthClear", "bool", "1");
	in("depthValue", "float", "1");
	in("darken", "bool", "0");
	in("darkenColor", "color", "000");
	in("multiply", "bool", "0");
	in("multiplyColor", "color", "fff");
	inEnum("viewMode", "surfaceViewMode");
	in("fov", "float", "90");
	in("zNear", "float", "0.01");
	in("zFar", "float", "1000");
	out("image", "image");
}

VfxNodeSurface::VfxNodeSurface()
	: VfxNodeBase()
	, surface(nullptr)
	, oldSurface(nullptr)
	, oldDepthTestEnabled(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Draw, kVfxPlugType_Draw);
	addInput(kInput_Format, kVfxPlugType_Int);
	addInput(kInput_Width, kVfxPlugType_Int);
	addInput(kInput_Height, kVfxPlugType_Int);
	addInput(kInput_Clear, kVfxPlugType_Bool);
	addInput(kInput_ClearColor, kVfxPlugType_Color);
	addInput(kInput_DepthClear, kVfxPlugType_Bool);
	addInput(kInput_DepthValue, kVfxPlugType_Float);
	addInput(kInput_Darken, kVfxPlugType_Bool);
	addInput(kInput_DarkenColor, kVfxPlugType_Color);
	addInput(kInput_Multiply, kVfxPlugType_Bool);
	addInput(kInput_MultiplyColor, kVfxPlugType_Color);
	addInput(kInput_ViewMode, kVfxPlugType_Int);
	addInput(kInput_FOV, kVfxPlugType_Float);
	addInput(kInput_ZNear, kVfxPlugType_Float);
	addInput(kInput_ZFar, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	
	// we implement custom draw to skip traversal when passthrough mode is set
	
	flags |= kFlag_CustomTraverseDraw;
}

VfxNodeSurface::~VfxNodeSurface()
{
	freeSurface();
}

void VfxNodeSurface::allocSurface(const int sx, const int sy, const SURFACE_FORMAT format, const bool withDepthBuffer)
{
	freeSurface();
	
	//
	
	surface = new Surface(sx, sy, withDepthBuffer, true, format);
	surface->clear();
	
	if (withDepthBuffer)
		surface->clearDepth(1.f);
}

void VfxNodeSurface::freeSurface()
{
	imageOutput.texture = 0;
	
	delete surface;
	surface = nullptr;
}

void VfxNodeSurface::tick(const float dt)
{
	if (isPassthrough)
	{
		freeSurface();
		return;
	}
	
	const Format format = (Format)getInputInt(kInput_Format, 0);
	const int sx = getInputInt(kInput_Width, VFXGRAPH_SX);
	const int sy = getInputInt(kInput_Height, VFXGRAPH_SY);
	const ViewMode viewMode = (ViewMode)getInputInt(kInput_ViewMode, 0);
	
	const SURFACE_FORMAT surfaceFormat = (format == kFormat_RGBA8) ? SURFACE_RGBA8 :  SURFACE_RGBA16F;
	
	const bool withDepthBuffer = (viewMode == kViewMode_Perspective);
	
	if (surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight() || withDepthBuffer != surface->hasDepthTexture() || surfaceFormat != surface->getFormat())
	{
		logDebug("allocating surface. withDepthBuffer=%d, format=%d", withDepthBuffer, surfaceFormat);
		
		allocSurface(sx, sy, surfaceFormat, withDepthBuffer);
	}
}

void VfxNodeSurface::customTraverseDraw(const int traversalId) const
{
	if (isPassthrough)
		return;
	
	for (auto predep : predeps)
	{
		if (predep->lastDrawTraversalId != traversalId)
			predep->traverseDraw(traversalId);
	}
}

void VfxNodeSurface::beforeDraw() const
{
	if (isPassthrough)
		return;
	
	const bool clear = getInputBool(kInput_Clear, true);
	const VfxColor * clearColor = getInputColor(kInput_ClearColor, nullptr);
	const bool depthClear = getInputBool(kInput_DepthClear, true);
	const float depthValue = getInputFloat(kInput_DepthValue, 1.f);
	const bool darken = getInputBool(kInput_Darken, false);
	const VfxColor * darkenColor = getInputColor(kInput_DarkenColor, nullptr);
	const bool multiply = getInputBool(kInput_Multiply, false);
	const VfxColor * multiplyColor = getInputColor(kInput_MultiplyColor, nullptr);
	const ViewMode viewMode = (ViewMode)getInputInt(kInput_ViewMode, 0);
	const float fov = getInputFloat(kInput_FOV, 90.f);
	const float zNear = getInputFloat(kInput_ZNear, .01f);
	const float zFar = getInputFloat(kInput_ZFar, 1000.f);
	
	oldSurface = g_currentVfxSurface;
	g_currentVfxSurface = surface;
	
	glGetIntegerv(GL_DEPTH_TEST, &oldDepthTestEnabled);
	
	pushSurface(surface);
	
	if (clear)
	{
		if (clearColor)
			surface->clearf(clearColor->r, clearColor->g, clearColor->b, clearColor->a);
		else
			surface->clear();
	}
	
	if (surface->getDepthTexture() != 0)
	{
		if (depthClear)
			surface->clearDepth(depthValue);
	}
	
	if (darken && darkenColor)
	{
		pushBlend(BLEND_SUBTRACT);
		setColorf(darkenColor->r, darkenColor->g, darkenColor->b, darkenColor->a);
		drawRect(0, 0, surface->getWidth(), surface->getHeight());
		popBlend();
	}
	
	if (multiply && multiplyColor)
	{
		pushBlend(BLEND_MUL);
		setColorf(multiplyColor->r, multiplyColor->g, multiplyColor->b, multiplyColor->a);
		drawRect(0, 0, surface->getWidth(), surface->getHeight());
		popBlend();
	}
	
	pushTransform();
	
	if (viewMode == kViewMode_Screen)
	{
		projectScreen2d();
		
		gxTranslatef(surface->getWidth()/2.f, surface->getHeight()/2.f, 0.f);
		
		glDisable(GL_DEPTH_TEST);
		checkErrorGL();
	}
	
	if (viewMode == kViewMode_Perspective)
	{
		projectPerspective3d(fov, zNear, zFar);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		checkErrorGL();
	}
	
	pushBlend(BLEND_ALPHA);
}

void VfxNodeSurface::afterDraw() const
{
	if (isPassthrough)
		return;
	
	popBlend();
	
	if (oldDepthTestEnabled)
	{
		glEnable(GL_DEPTH_TEST);
		checkErrorGL();
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		checkErrorGL();
	}
	
	popTransform();
	
	popSurface();
	
	g_currentVfxSurface = oldSurface;
	oldSurface = nullptr;
	
	//
	
	imageOutput.texture = surface->getTexture();
}
