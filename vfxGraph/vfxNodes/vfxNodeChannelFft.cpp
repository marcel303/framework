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

#include "fourier.h"
#include "vfxNodeChannelFft.h"

VFX_NODE_TYPE(VfxNodeChannelFft)
{
	typeName = "channel.fft";
	
	in("real", "channel");
	in("imag", "channel");
	in("reverse", "bool");
	in("normalize", "bool");
	out("real", "channel");
	out("imag", "channel");
}

VfxNodeChannelFft::VfxNodeChannelFft()
	: VfxNodeBase()
	, channelData()
	, realOutput()
	, imagOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Real, kVfxPlugType_Channel);
	addInput(kInput_Imag, kVfxPlugType_Channel);
	addInput(kInput_Reverse, kVfxPlugType_Bool);
	addInput(kInput_Normalize, kVfxPlugType_Bool);
	addOutput(kOutput_Real, kVfxPlugType_Channel, &realOutput);
	addOutput(kOutput_Imag, kVfxPlugType_Channel, &imagOutput);
}

void VfxNodeChannelFft::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelFft);
	
	const VfxChannel * realChannel = getInputChannel(kInput_Real, nullptr);
	const VfxChannel * imagChannel = getInputChannel(kInput_Imag, nullptr);
	const bool reverse = getInputBool(kInput_Reverse, false);
	const bool normalize = getInputBool(kInput_Normalize, false);
	
	const int sx = std::max(
		realChannel == nullptr ? 0 : realChannel->sx,
		imagChannel == nullptr ? 0 : imagChannel->sx);
	
	const int sy = std::max(
		realChannel == nullptr ? 0 : realChannel->sy,
		imagChannel == nullptr ? 0 : imagChannel->sy);
	
	if (isPassthrough)
	{
		channelData.free();

		if (realChannel == nullptr)
			realOutput.reset();
		else
			realOutput = *realChannel;

		if (imagChannel == nullptr)
			imagOutput.reset();
		else
			imagOutput = *imagChannel;
	}
	else if (sx == 0)
	{
		channelData.free();
		
		realOutput.reset();
		imagOutput.reset();
	}
	else
	{
		if (sy == 1)
		{
			const int transformSize = Fourier::upperPowerOf2(sx);
			const int numBits = Fourier::integerLog2(transformSize);
			
			channelData.allocOnSizeChange(transformSize * 2);
			
			float * __restrict real = channelData.data + 0 * transformSize;
			float * __restrict imag = channelData.data + 1 * transformSize;
			
			int xReversed[sx];
			
			for (int x = 0; x < sx; ++x)
				xReversed[x] = Fourier::reverseBits(x, numBits);
			
			if (realChannel == nullptr)
			{
				for (int x = 0; x < sx; ++x)
					real[xReversed[x]] = 0.f;
			}
			else
			{
				for (int x = 0; x < realChannel->sx; ++x)
					real[xReversed[x]] = realChannel->data[x];
				for (int x = realChannel->sx; x < sx; ++x)
					real[xReversed[x]] = 0.f;
			}
			
			if (imagChannel == nullptr)
			{
				for (int x = 0; x < sx; ++x)
					imag[xReversed[x]] = 0.f;
			}
			else
			{
				for (int x = 0; x < imagChannel->sx; ++x)
					imag[xReversed[x]] = imagChannel->data[x];
				for (int x = imagChannel->sx; x < sx; ++x)
					imag[xReversed[x]] = 0.f;
			}
			
			Fourier::fft1D(real, imag, sx, transformSize, reverse, normalize);
			
			const bool isContinuous = reverse ? true : false;
			
			realOutput.setData(real, isContinuous, transformSize);
			imagOutput.setData(imag, isContinuous, transformSize);
		}
		else
		{
			realOutput.reset();
			imagOutput.reset();
		}
	}
}

void VfxNodeChannelFft::getDescription(VfxNodeDescription & d)
{
	d.add("output channels:");
	d.add(realOutput);
	d.add(imagOutput);
}
