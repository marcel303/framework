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

#include "vfxNodeSampleImageCpu.h"
#include <cmath>

//

VFX_NODE_TYPE(VfxNodeSampleImageCpu)
{
	typeName = "image_cpu.sample";
	
	in("image", "image_cpu");
	in("x", "float", "0.5");
	in("y", "float", "0.5");
	in("filter", "bool", "1");
	in("clamp", "bool", "1");
	out("r", "float");
	out("g", "float");
	out("b", "float");
	out("a", "float");
	out("rgba", "channel");
}

//

VfxNodeSampleImageCpu::VfxNodeSampleImageCpu()
	: VfxNodeBase()
	, rgbaValueOutput()
	, rgbaChannelOutput()
{
	rgbaValueOutput[0] = 0.f;
	rgbaValueOutput[1] = 0.f;
	rgbaValueOutput[2] = 0.f;
	rgbaValueOutput[3] = 0.f;

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_Filter, kVfxPlugType_Bool);
	addInput(kInput_Clamp, kVfxPlugType_Bool);
	addOutput(kOutput_R, kVfxPlugType_Float, &rgbaValueOutput[0]);
	addOutput(kOutput_G, kVfxPlugType_Float, &rgbaValueOutput[1]);
	addOutput(kOutput_B, kVfxPlugType_Float, &rgbaValueOutput[2]);
	addOutput(kOutput_A, kVfxPlugType_Float, &rgbaValueOutput[3]);
	addOutput(kOutput_RGBA, kVfxPlugType_Channel, &rgbaChannelOutput);
}

void VfxNodeSampleImageCpu::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeSampleImageCpu);

	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const float xNorm = getInputFloat(kInput_X, .5f);
	const float yNorm = getInputFloat(kInput_Y, .5f);
	const bool filter = getInputBool(kInput_Filter, true);
	const bool clamp = getInputBool(kInput_Clamp, true);

	if (isPassthrough || image == nullptr || image->sx == 0 || image->sy == 0)
	{
		rgbaValueOutput[0] = 0.f;
		rgbaValueOutput[1] = 0.f;
		rgbaValueOutput[2] = 0.f;
		rgbaValueOutput[3] = 0.f;

		rgbaChannelOutput.reset();

		return;
	}
	
	if (filter)
	{
		if (clamp)
		{
			float xf = xNorm * (image->sx - 1);
			float yf = yNorm * (image->sy - 1);
			
			if (xf < 0)
				xf = 0;
			else if (xf >= image->sx)
				xf = image->sx - 1;
			
			if (yf < 0)
				yf = 0;
			else if (yf >= image->sy)
				yf = image->sy - 1;
			
			const int x1 = int(xf);
			const int y1 = int(yf);
			
			int x2 = x1 + 1;
			int y2 = y1 + 1;
			
			if (x2 >= image->sx)
				x2--;
			if (y2 >= image->sy)
				y2--;
			
			Assert(x1 >= 0 && x1 < image->sx);
			Assert(y1 >= 0 && y1 < image->sy);
			Assert(x2 >= 0 && x2 < image->sx);
			Assert(y2 >= 0 && y2 < image->sy);
			
			Assert(x1 <= x2);
			Assert(y1 <= y2);
			
			const float s2 = xf - x1;
			const float t2 = yf - y1;
			const float s1 = 1.f - s2;
			const float t1 = 1.f - t2;
			
			Assert(s1 >= 0.f && s1 <= 1.f);
			Assert(t1 >= 0.f && t1 <= 1.f);
			Assert(s2 >= 0.f && s2 <= 1.f);
			Assert(t2 >= 0.f && t2 <= 1.f);
			
			for (int i = 0; i < 4; ++i)
			{
				auto & c = image->channel[i];
				
				const int v00 = c.data[y1 * c.pitch + x1 * c.stride];
				const int v01 = c.data[y1 * c.pitch + x2 * c.stride];
				const int v10 = c.data[y2 * c.pitch + x1 * c.stride];
				const int v11 = c.data[y2 * c.pitch + x2 * c.stride];
				
				const float v0 = v00 * s1 + v01 * s2;
				const float v1 = v10 * s1 + v11 * s2;
				
				const float v = v0 * t1 + v1 * t2;
				
				rgbaValueOutput[i] = v / 255.f;
			}
		}
		else
		{
			const float xf = xNorm * image->sx;
			const float yf = yNorm * image->sy;
			
			const int xi = xf < 0 ? int(xf) - 1 : int(xf);
			const int yi = yf < 0 ? int(yf) - 1 : int(yf);
			
			Assert(xi <= xf);
			Assert(yi <= yf);
			
			const int dx = xi / image->sx;
			const int dy = yi / image->sy;
			
			const int nx = xi < 0 ? dx - 1 : dx;
			const int ny = yi < 0 ? dy - 1 : dy;
			
			const int x1 = xi - nx * image->sx;
			const int y1 = yi - ny * image->sy;
			
			const int x2 = (x1 + 1) % image->sx;
			const int y2 = (y1 + 1) % image->sy;
			
			Assert(x1 >= 0 && x1 < image->sx);
			Assert(y1 >= 0 && y1 < image->sy);
			Assert(x2 >= 0 && x2 < image->sx);
			Assert(y2 >= 0 && y2 < image->sy);
			
			const float s2 = xf - xi;
			const float t2 = yf - yi;
			const float s1 = 1.f - s2;
			const float t1 = 1.f - t2;
			
			Assert(s1 >= 0.f && s1 <= 1.f);
			Assert(t1 >= 0.f && t1 <= 1.f);
			Assert(s2 >= 0.f && s2 <= 1.f);
			Assert(t2 >= 0.f && t2 <= 1.f);
			
			for (int i = 0; i < 4; ++i)
			{
				auto & c = image->channel[i];
				
				const int v00 = c.data[y1 * c.pitch + x1 * c.stride];
				const int v01 = c.data[y1 * c.pitch + x2 * c.stride];
				const int v10 = c.data[y2 * c.pitch + x1 * c.stride];
				const int v11 = c.data[y2 * c.pitch + x2 * c.stride];
				
				const float v0 = v00 * s1 + v01 * s2;
				const float v1 = v10 * s1 + v11 * s2;
				
				const float v = v0 * t1 + v1 * t2;
				
				rgbaValueOutput[i] = v / 255.f;
			}
		}
	}
	else
	{
		if (clamp)
		{
			int x = int(xNorm * image->sx);
			int y = int(yNorm * image->sy);
			
			if (x < 0)
				x = 0;
			else if (x >= image->sx)
				x = image->sx - 1;
			
			if (y < 0)
				y = 0;
			else if (y >= image->sy)
				y = image->sy - 1;
			
			Assert(x >= 0 && x < image->sx);
			Assert(y >= 0 && y < image->sy);
			
			for (int i = 0; i < 4; ++i)
			{
				auto & c = image->channel[i];
				
				rgbaValueOutput[i] = c.data[y * c.pitch + x * c.stride] / 255.f;
			}
		}
		else
		{
			const float xf = xNorm * image->sx;
			const float yf = yNorm * image->sy;
			
			const int xi = xf < 0 ? int(xf) - 1 : int(xf);
			const int yi = yf < 0 ? int(yf) - 1 : int(yf);
			
			const int dx = xi / image->sx;
			const int dy = yi / image->sy;
			
			const int nx = xi < 0 ? dx - 1 : dx;
			const int ny = yi < 0 ? dy - 1 : dy;
			
			const int x = xi - nx * image->sx;
			const int y = yi - ny * image->sy;
			
			Assert(x >= 0 && x < image->sx);
			Assert(y >= 0 && y < image->sy);
			
			for (int i = 0; i < 4; ++i)
			{
				auto & c = image->channel[i];
				
				rgbaValueOutput[i] = c.data[y * c.pitch + x * c.stride] / 255.f;
			}
		}
	}
	
	rgbaChannelOutput.setData(rgbaValueOutput, false, 4);
}
