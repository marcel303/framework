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
#include "vfxNodeComposite.h"
#include "vfxTypes.h"

extern const int GFX_SX;
extern const int GFX_SY;

VFX_NODE_TYPE(VfxNodeComposite)
{
	typeName = "draw.composite";
	
	in("image1", "image");
	in("transform1", "transform");
	in("image2", "image");
	in("transform2", "transform");
	in("image3", "image");
	in("transform3", "transform");
	in("image4", "image");
	in("transform4", "transform");
	out("image", "image");
}

VfxNodeComposite::VfxNodeComposite()
	: VfxNodeBase()
	, surface(nullptr)
	, image()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image1, kVfxPlugType_Image);
	addInput(kInput_Transform1, kVfxPlugType_Transform);
	addInput(kInput_Image2, kVfxPlugType_Image);
	addInput(kInput_Transform2, kVfxPlugType_Transform);
	addInput(kInput_Image3, kVfxPlugType_Image);
	addInput(kInput_Transform3, kVfxPlugType_Transform);
	addInput(kInput_Image4, kVfxPlugType_Image);
	addInput(kInput_Transform4, kVfxPlugType_Transform);
	addOutput(kOutput_Image, kVfxPlugType_Image, &image);
	
	surface = new Surface(GFX_SX, GFX_SY, true);
}

VfxNodeComposite::~VfxNodeComposite()
{
	delete surface;
	surface = nullptr;
}

void VfxNodeComposite::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeComposite);
	vfxGpuTimingBlock(VfxNodeComposite);
	
	pushSurface(surface);
	{
		surface->clear();
		
		const int kNumImages = 4;
		
		const VfxImageBase * images[kNumImages] =
		{
			getInputImage(kInput_Image1, nullptr),
			getInputImage(kInput_Image2, nullptr),
			getInputImage(kInput_Image3, nullptr),
			getInputImage(kInput_Image4, nullptr)
		};
		
		const VfxTransform defaultTransform;
		
		const VfxTransform * transforms[kNumImages] =
		{
			&getInputTransform(kInput_Transform1, defaultTransform),
			&getInputTransform(kInput_Transform2, defaultTransform),
			&getInputTransform(kInput_Transform3, defaultTransform),
			&getInputTransform(kInput_Transform4, defaultTransform)
		};
		
		for (int i = 0; i < kNumImages; ++i)
		{
			if (images[i] == nullptr)
				continue;
			
			gxPushMatrix();
			{
				gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
				gxMultMatrixf(transforms[i]->matrix.m_v);
				gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0.f);
				
				gxSetTexture(images[i]->getTexture());
				{
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				gxSetTexture(0);
			}
			gxPopMatrix();
		}
	}
	popSurface();
	
	image.texture = surface->getTexture();
}
