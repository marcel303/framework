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
#include "vfxGraph.h"
#include "vfxNodeDrawImage.h"

VFX_ENUM_TYPE(drawImageSizeMode)
{
	elem("fill");
	elem("contain");
	elem("dontScale");
	elem("stretch");
	elem("fitX");
	elem("fitY");
}

VFX_NODE_TYPE(VfxNodeDrawImage)
{
	typeName = "draw.image";
	
	in("image", "image");
	inEnum("sizeMode", "drawImageSizeMode");
	in("opacity", "float", "1");
	out("any", "draw", "draw");
}

VfxNodeDrawImage::VfxNodeDrawImage()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_SizeMode, kVfxPlugType_Int);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, this);
}

void VfxNodeDrawImage::draw() const
{
	if (isPassthrough)
		return;

	vfxCpuTimingBlock(VfxNodeDrawImage);

	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const SizeMode sizeMode = (SizeMode)getInputInt(kInput_SizeMode, 0);
	const float opacity = getInputFloat(kInput_Opacity, 1.f);
	
	if (image != nullptr)
	{
		vfxGpuTimingBlock(VfxNodeDrawImage);
		
		const TRANSFORM transform = getTransform();
		
		if (transform == TRANSFORM_SCREEN)
		{
			float scaleX = 1.f;
			float scaleY = 1.f;
			
			const int imageSx = image->getSx();
			const int imageSy = image->getSy();
			
			const float viewSx = g_currentVfxSurface->getWidth();
			const float viewSy = g_currentVfxSurface->getHeight();
			
			const float fillScaleX = viewSx / float(imageSx);
			const float fillScaleY = viewSy / float(imageSy);
			
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
			
			gxSetTexture(image->getTexture());
			{
				gxPushMatrix();
				{
					gxScalef(scaleX, scaleY, 1.f);
					
					setColorf(1.f, 1.f, 1.f, opacity);
					drawRect(-imageSx/2.f, -imageSy/2.f, +imageSx/2.f, +imageSy/2.f);
				}
				gxPopMatrix();
			}
			gxSetTexture(0);
		}
		else
		{
			float scaleX = 1.f;
			float scaleY = 1.f;
			
			const int imageSx = image->getSx();
			const int imageSy = image->getSy();
			
			const float fillScaleX = 2.f / float(imageSx);
			const float fillScaleY = 2.f / float(imageSy);
			
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
			
			gxSetTexture(image->getTexture());
			{
				gxPushMatrix();
				{
					gxScalef(scaleX, scaleY, 1.f);
					
					setColorf(1.f, 1.f, 1.f, opacity);
					drawRect(-imageSx/2.f, -imageSy/2.f, +imageSx/2.f, +imageSy/2.f);
				}
				gxPopMatrix();
			}
			gxSetTexture(0);
		}
	}
}
