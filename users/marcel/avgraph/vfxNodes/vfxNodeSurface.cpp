#include "framework.h"
#include "vfxNodeSurface.h"

extern const int GFX_SX;
extern const int GFX_SY;

VfxNodeSurface::VfxNodeSurface()
	: VfxNodeBase()
	, surface(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DontCare, kVfxPlugType_DontCare);
	addInput(kInput_Clear, kVfxPlugType_Bool);
	addInput(kInput_ClearColor, kVfxPlugType_Color);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);

	surface = new Surface(GFX_SX, GFX_SY, true);

	imageOutput.texture = surface->getTexture();
}

VfxNodeSurface::~VfxNodeSurface()
{
	delete surface;
	surface = nullptr;
}

void VfxNodeSurface::beforeTick()
{
	const bool clear = getInputBool(kInput_Clear, true);
	const VfxColor * clearColor = getInputColor(kInput_ClearColor, nullptr);
	
	pushSurface(surface);
	
	if (clear)
	{
		if (clearColor)
			surface->clearf(clearColor->r, clearColor->g, clearColor->b, clearColor->a);
		else
			surface->clear();
	}
}

void VfxNodeSurface::afterTick()
{
	popSurface();
}
