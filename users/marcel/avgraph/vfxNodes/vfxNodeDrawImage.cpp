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
#include "vfxNodeDrawImage.h"

extern const int GFX_SX;
extern const int GFX_SY;

VFX_ENUM_TYPE(drawImageSizeMode)
{
	elem("fill");
	elem("contain");
	elem("dontScale");
	elem("stretch");
	elem("fitX");
	elem("fitY");
}

VFX_NODE_TYPE(draw_image, VfxNodeDrawImage)
{
	typeName = "draw.image";
	
	in("image", "image");
	inEnum("sizeMode", "drawImageSizeMode");
	in("opacity", "float", "1");
	out("any", "any");
}

VfxNodeDrawImage::VfxNodeDrawImage()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_SizeMode, kVfxPlugType_Int);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, nullptr);
}

void VfxNodeDrawImage::draw() const
{
	if (isPassthrough)
		return;
		
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const SizeMode sizeMode = (SizeMode)getInputInt(kInput_SizeMode, 0);
	const float opacity = getInputFloat(kInput_Opacity, 1.f);
	
	if (image != nullptr)
	{
		float scaleX = 1.f;
		float scaleY = 1.f;
		
		const int imageSx = image->getSx();
		const int imageSy = image->getSy();
		
		const float fillScaleX = GFX_SX / float(imageSx);
		const float fillScaleY = GFX_SY / float(imageSy);
		
		if (sizeMode == kSizeMode_Fill)
		{
			const float scale = fmaxf(fillScaleX, fillScaleY);
			
			scaleX = scale;
			scaleY = scale;
		}
		else if (sizeMode == kSizeMode_Contain)
		{
			const float scale = fminf(fillScaleX, fillScaleY);
			
			scaleX = scale;
			scaleY = scale;
		}
		else if (sizeMode == kSizeMode_DontScale)
		{
			scaleX = 1.f;
			scaleY = 1.f;
		}
		else if (sizeMode == kSizeMode_Stretch)
		{
			scaleX = fillScaleX;
			scaleY = fillScaleY;
		}
		else if (sizeMode == kSizeMode_FitX)
		{
			const float scale = fillScaleX;
			
			scaleX = scale;
			scaleY = scale;
		}
		else if (sizeMode == kSizeMode_FitY)
		{
			const float scale = fillScaleY;
			
			scaleX = scale;
			scaleY = scale;
		}
		else
		{
			Assert(false);
		}
		
		const float offsetX = (GFX_SX - imageSx * scaleX) / 2.f;
		const float offsetY = (GFX_SY - imageSy * scaleY) / 2.f;
		
		gxSetTexture(image->getTexture());
		{
			gxPushMatrix();
			{
				gxTranslatef(offsetX, offsetY, 0.f);
				gxScalef(scaleX, scaleY, 1.f);
				
				setColorf(1.f, 1.f, 1.f, opacity);
				drawRect(0, 0, imageSx, imageSy);
			}
			gxPopMatrix();
		}
		gxSetTexture(0);
	}
}
