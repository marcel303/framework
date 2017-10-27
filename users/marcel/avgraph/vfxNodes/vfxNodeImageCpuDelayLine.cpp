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

#if ENABLE_TURBOJPEG

#include "imageCpuDelayLine.h"
#include "vfxNodeImageCpuDelayLine.h"
#include <algorithm>
#include <cmath>

VFX_NODE_TYPE(image_cpu_delay, VfxNodeImageCpuDelayLine)
{
	typeName = "image_cpu.delay";
	
	in("image", "image_cpu");
	in("maxDelay", "float");
	in("compress", "bool");
	in("jpegQuality", "float", "0.85");
	in("delay1", "float", "-1");
	in("delay2", "float", "-1");
	in("delay3", "float", "-1");
	in("delay4", "float", "-1");
	out("image1", "image_cpu");
	out("image2", "image_cpu");
	out("image3", "image_cpu");
	out("image4", "image_cpu");
}

VfxNodeImageCpuDelayLine::VfxNodeImageCpuDelayLine()
	: VfxNodeBase()
	, imageData()
	, delayLine(nullptr)
{
	delayLine = new ImageCpuDelayLine();

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_MaxDelay, kVfxPlugType_Float);
	addInput(kInput_CompressionEnabled, kVfxPlugType_Bool);
	addInput(kInput_JpegQualityLevel, kVfxPlugType_Float);
	addInput(kInput_Delay1, kVfxPlugType_Float);
	addInput(kInput_Delay2, kVfxPlugType_Float);
	addInput(kInput_Delay3, kVfxPlugType_Float);
	addInput(kInput_Delay4, kVfxPlugType_Float);
	addOutput(kOutput_Image1, kVfxPlugType_ImageCpu, &imageData[0].image);
	addOutput(kOutput_Image2, kVfxPlugType_ImageCpu, &imageData[1].image);
	addOutput(kOutput_Image3, kVfxPlugType_ImageCpu, &imageData[2].image);
	addOutput(kOutput_Image4, kVfxPlugType_ImageCpu, &imageData[3].image);
	
	delayLine->init(0, 1024 * 512);
}

VfxNodeImageCpuDelayLine::~VfxNodeImageCpuDelayLine()
{
	delayLine->shut();
	
	delete delayLine;
	delayLine = nullptr;
}

void VfxNodeImageCpuDelayLine::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageCpuDelayLine);
	
	const int maxDelay = 1 + std::max(0, (int)std::ceil(getInputFloat(kInput_MaxDelay, 0.f)));
	const bool compressionEnabled = getInputBool(kInput_CompressionEnabled, false);
	const int jpegQualityLevel = std::max(0, std::min(100, (int)std::round(getInputFloat(kInput_JpegQualityLevel, .85f) * 100.f)));
	
	const int delay1 = (int)std::round(getInputFloat(kInput_Delay1, -1.f) * maxDelay);
	const int delay2 = (int)std::round(getInputFloat(kInput_Delay2, -1.f) * maxDelay);
	const int delay3 = (int)std::round(getInputFloat(kInput_Delay3, -1.f) * maxDelay);
	const int delay4 = (int)std::round(getInputFloat(kInput_Delay4, -1.f) * maxDelay);
	
	// tick the delay line to commmit pending compression work
	
	delayLine->tick();
	
	// set delay line length
	
	if (maxDelay != delayLine->getLength())
	{
		delayLine->setLength(maxDelay);
	}
	
	if (delayLine->getLength() > 0)
	{
		const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);

		if (image != nullptr)
		{
			delayLine->add(*image, jpegQualityLevel, 0.0, compressionEnabled);
		}
		
		const int offset[4] =
		{
			std::max(0, std::min(delayLine->getLength() - 1, delay1)),
			std::max(0, std::min(delayLine->getLength() - 1, delay2)),
			std::max(0, std::min(delayLine->getLength() - 1, delay3)),
			std::max(0, std::min(delayLine->getLength() - 1, delay4))
		};

		for (int i = 0; i < 4; ++i)
		{
			if (tryGetInput(kInput_Delay1 + i)->isConnected())
			{
				if (delayLine->get(offset[i], imageData[i]) == false)
				{
					imageData[i].free();
				}
			}
			else
			{
				imageData[i].free();
			}
		}
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			imageData[i].free();
		}
	}
}

void VfxNodeImageCpuDelayLine::getDescription(VfxNodeDescription & d)
{
	ImageCpuDelayLine::MemoryUsage memoryUsage = delayLine->getMemoryUsage();
	
	d.add("memory usage. total: %.2fMb, history:%.2fMb (%d/%d)", memoryUsage.numBytes/(1024.0*1024.0), memoryUsage.numHistoryBytes/(1024.0*1024.0), memoryUsage.historySize, delayLine->maxHistorySize);
	d.newline();
	
	for (int i = 0; i < 4; ++i)
	{
		char name[64];
		sprintf(name, "output image %d", i + 1);
		
		d.add(name, imageData[i].image);
		d.newline();
	}
}

#endif
