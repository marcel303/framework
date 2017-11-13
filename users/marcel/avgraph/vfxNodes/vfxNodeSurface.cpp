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

// todo : implement custom draw to skip traversal when passthrough mode is set

#include "framework.h"
#include "vfxNodeSurface.h"

extern const int GFX_SX;
extern const int GFX_SY;

VFX_ENUM_TYPE(surfaceViewMode)
{
	elem("screen");
	elem("perspective");
}

VFX_NODE_TYPE(VfxNodeSurface)
{
	typeName = "draw.surface";
	
	in("source", "any");
	in("clear", "bool", "1");
	in("clearColor", "color", "fff");
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
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DontCare, kVfxPlugType_DontCare);
	addInput(kInput_Clear, kVfxPlugType_Bool);
	addInput(kInput_ClearColor, kVfxPlugType_Color);
	addInput(kInput_Darken, kVfxPlugType_Bool);
	addInput(kInput_DarkenColor, kVfxPlugType_Color);
	addInput(kInput_Multiply, kVfxPlugType_Bool);
	addInput(kInput_MultiplyColor, kVfxPlugType_Color);
	addInput(kInput_ViewMode, kVfxPlugType_Int);
	addInput(kInput_FOV, kVfxPlugType_Float);
	addInput(kInput_ZNear, kVfxPlugType_Float);
	addInput(kInput_ZFar, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	
	flags |= kFlag_CustomTraverseDraw;
}

VfxNodeSurface::~VfxNodeSurface()
{
	freeSurface();
}

void VfxNodeSurface::allocSurface(const bool withDepthBuffer)
{
	freeSurface();
	
	//
	
	surface = new Surface(GFX_SX, GFX_SY, withDepthBuffer, false, SURFACE_RGBA16F);
	surface->clear();
	surface->clearDepth(1.f);
	
	imageOutput.texture = surface->getTexture();
}

void VfxNodeSurface::freeSurface()
{
	imageOutput.texture = 0;
	
	delete surface;
	surface = nullptr;
}

void VfxNodeSurface::init(const GraphNode & node)
{
	const ViewMode viewMode = (ViewMode)getInputInt(kInput_ViewMode, 0);
	
	const bool withDepthBuffer = (viewMode == kViewMode_Perspective);
	
	if (surface == nullptr || withDepthBuffer != surface->hasDepthTexture())
		allocSurface(withDepthBuffer);
}

void VfxNodeSurface::tick(const float dt)
{
	if (isPassthrough)
	{
		freeSurface();
		return;
	}
	
	const ViewMode viewMode = (ViewMode)getInputInt(kInput_ViewMode, 0);
	
	const bool withDepthBuffer = (viewMode == kViewMode_Perspective);
	
	if (surface == nullptr || withDepthBuffer != surface->hasDepthTexture())
		allocSurface(withDepthBuffer);
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
	const bool darken = getInputBool(kInput_Darken, false);
	const VfxColor * darkenColor = getInputColor(kInput_DarkenColor, nullptr);
	const bool multiply = getInputBool(kInput_Multiply, false);
	const VfxColor * multiplyColor = getInputColor(kInput_MultiplyColor, nullptr);
	const ViewMode viewMode = (ViewMode)getInputInt(kInput_ViewMode, 0);
	const float fov = getInputFloat(kInput_FOV, 90.f);
	const float zNear = getInputFloat(kInput_ZNear, .01f);
	const float zFar = getInputFloat(kInput_ZFar, 1000.f);
	
	pushSurface(surface);
	
	if (clear)
	{
		if (clearColor)
			surface->clearf(clearColor->r, clearColor->g, clearColor->b, clearColor->a);
		else
			surface->clear();
		
		if (viewMode == kViewMode_Perspective)
			surface->clearDepth(1.f);
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
		projectScreen2d();
	if (viewMode == kViewMode_Perspective)
		projectPerspective3d(fov, zNear, zFar);
	
	if (viewMode == kViewMode_Perspective)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}
}

void VfxNodeSurface::afterDraw() const
{
	if (isPassthrough)
		return;
	
	const ViewMode viewMode = (ViewMode)getInputInt(kInput_ViewMode, 0);
	
	if (viewMode == kViewMode_Perspective)
		glDisable(GL_DEPTH_TEST);
	
	popTransform();
	
	popSurface();
}
