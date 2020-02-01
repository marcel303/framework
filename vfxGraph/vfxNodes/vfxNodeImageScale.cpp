/*
	Copyright (C) 2020 Marcel Smit
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
#include "vfxNodeImageScale.h"
#include <math.h>

VFX_NODE_TYPE(VfxNodeImageScale)
{
	typeName = "image.scale";
	
	in("image", "image");
	in("scale", "float", "1");
	in("maxWidth", "float", "-1");
	in("maxHeight", "float", "-1");
	out("image", "image");
}

// todo : move this to its own source file, to make sure conglomerate build keeps working once we convert other nodes to use this function too
static SURFACE_FORMAT textureFormatToSurfaceFormat(const GX_TEXTURE_FORMAT textureFormat)
{
	switch (textureFormat)
	{
	case GX_R8_UNORM:
		return SURFACE_R8;
	case GX_RG8_UNORM:
		return SURFACE_RG8;
	case GX_RGBA8_UNORM:
		return SURFACE_RGBA8;
	case GX_R16_UNORM:
		logWarning("translating R16 texture format to R8 surface format!");
		return SURFACE_R8;
	case GX_R16_FLOAT:
		return SURFACE_R16F;
	case GX_RGBA16_FLOAT:
		return SURFACE_RGBA16F;
	case GX_R32_FLOAT:
		return SURFACE_R32F;
	case GX_RGBA32_FLOAT:
		return SURFACE_RGBA32F;
	
	default:
		Assert(false);
		return SURFACE_RGBA8;
	}
}

VfxNodeImageScale::VfxNodeImageScale()
	: VfxNodeBase()
	, surface(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addInput(kInput_MaxSx, kVfxPlugType_Float);
	addInput(kInput_MaxSy, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeImageScale::~VfxNodeImageScale()
{
	freeImage();
}
void VfxNodeImageScale::draw() const
{
	vfxCpuTimingBlock(VfxNodeImageScale);
	
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const float _scale = getInputFloat(kInput_Scale, 1.f);
	const float maxSx = getInputFloat(kInput_MaxSx, -1.f);
	const float maxSy = getInputFloat(kInput_MaxSy, -1.f);

	if (image == nullptr || image->getSx() == 0 || image->getSy() == 0)
	{
		freeImage();
	}
	else
	{
		vfxGpuTimingBlock(VfxNodeImageScale);
		
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
			const int sx = int(ceilf(image->getSx() * scale));
			const int sy = int(ceilf(image->getSy() * scale));
			const GX_TEXTURE_FORMAT format = (GX_TEXTURE_FORMAT)image->getTextureFormat();
			const SURFACE_FORMAT surfaceFormat = textureFormatToSurfaceFormat(format);
			
			if (surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight() || surfaceFormat != surface->getFormat())
			{
				allocateImage(sx, sy, surfaceFormat);
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

void VfxNodeImageScale::allocateImage(const int sx, const int sy, const SURFACE_FORMAT format) const
{
	freeImage();
	
	surface = new Surface(sx, sy, false, false, format, 1);
	
	imageOutput.texture = surface->getTexture();
}

void VfxNodeImageScale::freeImage() const
{
	delete surface;
	surface = nullptr;

	imageOutput.texture = 0;
}
