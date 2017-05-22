#include "vfxNodeSpectrum1D.h"

VfxNodeSpectrum1D::VfxNodeSpectrum1D()
	: VfxNodeBase()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Buffer, kVfxPlugType_Int);
	addInput(kInput_Size, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeSpectrum1D::~VfxNodeSpectrum1D()
{
	delete surface;
	surface = nullptr;
}

void VfxNodeSpectrum1D::tick(const float dt)
{
	// todo : analyze buffer. create/update texture

	const int buffer = getInputInt(kInput_Buffer, 0);
	const int windowSize = getInputInt(kInput_Size, 64);
	
	if (windowSize != surface->getWidth())
	{
		delete surface;
		surface = nullptr;
		
		surface = new Surface(windowSize, 1, false, false, SURFACE_R32F);
	}
	
	pushSurface(surface);
	{
		pushBlend(BLEND_OPAQUE);
		setColor(rand() % 256, rand() % 256, rand() % 256);
		drawRect(0, 0, surface->getWidth(), surface->getHeight());
		popBlend();
	}
	popSurface();
	
	imageOutput.texture = surface->getTexture();
}

void VfxNodeSpectrum1D::init(const GraphNode & node)
{
	const int windowSize = getInputInt(kInput_Size, 64);
	
	surface = new Surface(windowSize, 1, false, false, SURFACE_R32F);
}
