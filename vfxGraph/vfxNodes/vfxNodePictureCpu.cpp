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

#include "image.h"
#include "vfxNodePictureCpu.h"

VFX_NODE_TYPE(VfxNodePictureCpu)
{
	typeName = "picture_cpu";
	
	in("source", "string");
	out("image", "image_cpu");
}

VfxNodePictureCpu::VfxNodePictureCpu()
	: VfxNodeBase()
	, imageData()
	, currentFilename()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageData.image);
}

VfxNodePictureCpu::~VfxNodePictureCpu()
{
	setImage(nullptr);
}

void VfxNodePictureCpu::init(const GraphNode & node)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (isPassthrough == false && filename != nullptr)
	{
		setImage(filename);
	}
}

void VfxNodePictureCpu::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodePictureCpu);
	
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (isPassthrough || filename == nullptr)
	{
		setImage(nullptr);
	}
	else if (filename != currentFilename)
	{
		setImage(filename);
	}
}

void VfxNodePictureCpu::setImage(const char * filename)
{
	imageData.free();
	
	//
	
	if (filename != nullptr)
	{
		currentFilename = filename;
		
		ImageData * loadedImageData = loadImage(filename);

		if (loadedImageData != nullptr)
		{
			imageData.alloc(loadedImageData->sx, loadedImageData->sy, 4);
			
			VfxImageCpu::deinterleave4(
				(uint8_t*)loadedImageData->imageData,
				loadedImageData->sx,
				loadedImageData->sy,
				4,
				0,
				imageData.image.channel[0],
				imageData.image.channel[1],
				imageData.image.channel[2],
				imageData.image.channel[3]);
			
			delete loadedImageData;
			loadedImageData = nullptr;
		}
	}
	else
	{
		currentFilename.clear();
	}
}

void VfxNodePictureCpu::getDescription(VfxNodeDescription & d)
{
	d.add("output image", imageData.image);
}
