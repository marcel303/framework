#include "framework.h"
#include "vfxNodeImageDownsample.h"

VfxNodeImageDownsample::VfxNodeImageDownsample()
	: VfxNodeBase()
	, surface(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_DownsampleSize, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeImageDownsample::~VfxNodeImageDownsample()
{
	freeImage();
}

void VfxNodeImageDownsample::tick(const float dt)
{
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const DownsampleSize downsampleSize = (DownsampleSize)getInputInt(kInput_DownsampleSize, kDownsampleSize_2x2);

	if (image == nullptr || image->getSx() == 0 || image->getSy() == 0)
	{
		freeImage();
	}
	else
	{
		const int pixelSize = downsampleSize == kDownsampleSize_2x2 ? 2 : 4;

		const int downsampledSx = std::max(1, image->getSx() / pixelSize);
		const int downsampledSy = std::max(1, image->getSy() / pixelSize);

		if (surface == nullptr || downsampledSx != surface->getWidth() || downsampledSy != surface->getHeight())
		{
			allocateImage(downsampledSx, downsampledSy);
		}

		pushSurface(surface);
		{
			pushBlend(BLEND_OPAQUE);
			setColor(colorWhite);
			gxSetTexture(image->getTexture());
			drawRect(0, 0, surface->getWidth(), surface->getHeight());
			gxSetTexture(0);
			popBlend();
		}
		popSurface();
		
		imageOutput.texture = surface->getTexture();
	}
}

void VfxNodeImageDownsample::allocateImage(const int sx, const int sy)
{
	freeImage();

	// todo : use the correct surface format
	
	surface = new Surface(sx, sy, false, false, SURFACE_RGBA8);
}

void VfxNodeImageDownsample::freeImage()
{
	delete surface;
	surface = nullptr;

	imageOutput.texture = 0;
}

