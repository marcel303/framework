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

#include "Calc.h"
#include "vfxNodeImageCpuCrop.h"
#include <cmath>

VFX_NODE_TYPE(VfxNodeImageCpuCrop)
{
	typeName = "image_cpu.crop";
	
	in("image", "image_cpu");
	in("amount", "float");
	in("amount_x", "float", "-1");
	in("amount_y", "float", "-1");
	in("align_x", "float", "0.5");
	in("align_y", "float", "0.5");
	out("image", "image_cpu");
}

VfxNodeImageCpuCrop::VfxNodeImageCpuCrop()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Amount, kVfxPlugType_Float);
	addInput(kInput_AmountX, kVfxPlugType_Float);
	addInput(kInput_AmountY, kVfxPlugType_Float);
	addInput(kInput_AlignX, kVfxPlugType_Float);
	addInput(kInput_AlignY, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageOutput);
}

void VfxNodeImageCpuCrop::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuCrop);
	
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const float amount = getInputFloat(kInput_Amount, 0.f);
	const float amountX = getInputFloat(kInput_AmountX, -1.f);
	const float amountY = getInputFloat(kInput_AmountY, -1.f);
	const float alignX = getInputFloat(kInput_AlignX, .5f);
	const float alignY = getInputFloat(kInput_AlignY, .5f);
	
	if (image == nullptr || image->sx == 0 || image->sy == 0 || image->numChannels == 0)
	{
		imageOutput.reset();
	}
	else if (isPassthrough)
	{
		imageOutput = *image;
	}
	else
	{
		const int sx = Calc::Clamp(int(std::round(image->sx * (1.0 - (amountX >= 0.f ? amountX : amount)))), 0, image->sx);
		const int sy = Calc::Clamp(int(std::round(image->sy * (1.0 - (amountY >= 0.f ? amountY : amount)))), 0, image->sy);
	#if 1
		const int ox = 0; // fixme : shifting x voids guarantee data is sy * pitch number of bytes
	#else
		const int ox = Calc::Clamp(int(std::round(image->sx - sx) * alignX), 0, image->sx - sx);
	#endif
		const int oy = Calc::Clamp(int(std::round(image->sy - sy) * alignY), 0, image->sy - sy);
		
		Assert(ox + sx <= image->sx);
		Assert(oy + sy <= image->sy);
		
		imageOutput = *image;

		imageOutput.sx = sx;
		imageOutput.sy = sy;
		
		for (int i = 0; i < imageOutput.numChannels; ++i)
		{
			imageOutput.channel[i].data += imageOutput.channel[i].pitch * oy + ox;
		}
	}
}

void VfxNodeImageCpuCrop::getDescription(VfxNodeDescription & d)
{
	d.add("output image", imageOutput);
}
