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
#include "vfxNodeImageDownsample.h"

// todo : implement single channel downsample
// todo : add setSwizzle method to Surface class
// todo : swizzle Surface class to RED, RED, RED, ONE for single channel formats ?

VfxNodeImageDownsample::VfxNodeImageDownsample()
	: VfxNodeBase()
	, surface(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addInput(kInput_MaxSx, kVfxPlugType_Float);
	addInput(kInput_MaxSy, kVfxPlugType_Float);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeImageDownsample::~VfxNodeImageDownsample()
{
	freeImage();
}

void VfxNodeImageDownsample::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageDownsample);
	
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const float _scale = getInputFloat(kInput_Scale, 1.f);
	const float maxSx = getInputFloat(kInput_MaxSx, -1.f);
	const float maxSy = getInputFloat(kInput_MaxSy, -1.f);
	
	const DownsampleChannel channel = (DownsampleChannel)getInputInt(kInput_Channel, kDownsampleChannel_All);

	if (image == nullptr || image->getSx() == 0 || image->getSy() == 0)
	{
		freeImage();
	}
	else
	{
		vfxGpuTimingBlock(VfxNodeImageDownsample);
		
		float scale = _scale;;
		
		if (tryGetInput(kInput_MaxSx)->isConnected())
		{
			const float scaleX = maxSx / image->getSx();
			
			scale = std::min(scale, scaleX);
		}
		
		if (tryGetInput(kInput_MaxSy)->isConnected())
		{
			const float scaleY = maxSy / image->getSy();
			
			scale = std::min(scale, scaleY);
		}
		
		if (scale <= 0.f)
		{
			freeImage();
		}
		else
		{
			const int sx = int(std::ceil(image->getSx() * scale));
			const int sy = int(std::ceil(image->getSy() * scale));
			
			if (surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight())
			{
				allocateImage(sx, sy);
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
			
			Assert(imageOutput.texture == surface->getTexture());
		}
	}
}

void VfxNodeImageDownsample::allocateImage(const int sx, const int sy)
{
	freeImage();

	// todo : use the correct surface format
	
	surface = new Surface(sx, sy, false, false, SURFACE_RGBA8);
	
	imageOutput.texture = surface->getTexture();
}

void VfxNodeImageDownsample::freeImage()
{
	delete surface;
	surface = nullptr;

	imageOutput.texture = 0;
}

