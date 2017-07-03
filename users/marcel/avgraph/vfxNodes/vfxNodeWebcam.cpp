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

#include "macWebcam.h"
#include "vfxNodeWebcam.h"
#include <GL/glew.h>

VFX_NODE_TYPE(webcam, VfxNodeWebcam)
{
	typeName = "webcam";
	
	in("device", "int");
	out("image", "image");
	out("image_mem", "image_cpu");
}

VfxNodeWebcam::VfxNodeWebcam()
	: VfxNodeBase()
	, currentDeviceIndex(-1)
	, webcam(nullptr)
	, lastImageIndex(-1)
	, texture()
	, imageOutput()
	, imageCpuOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceIndex, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	addOutput(kOutput_ImageCpu, kVfxPlugType_ImageCpu, &imageCpuOutput);
}

VfxNodeWebcam::~VfxNodeWebcam()
{
	freeImage();
	
	delete webcam;
	webcam = nullptr;
}

void VfxNodeWebcam::tick(const float dt)
{
	vfxGpuTimingBlock(VfxNodeWebcam);
	
	const int deviceIndex = getInputInt(kInput_DeviceIndex, 0);
	
	if (deviceIndex != currentDeviceIndex)
	{
		currentDeviceIndex = deviceIndex;

		webcam = new MacWebcam();

		if (webcam->init() == false)
		{
			webcam->shut();

			delete webcam;
			webcam = nullptr;
		}
	}

	if (webcam == nullptr)
	{
		freeImage();
	}
	else
	{
		webcam->tick();
		
		if (webcam->image/* && webcam->image->index != lastImageIndex*/)
		{
			const bool wantsTexture = outputs[kOutput_Image].isReferenced();
			
			if (wantsTexture)
			{
				vfxGpuTimingBlock(VfxNodeWebcam);
				
				if (texture.isChanged(webcam->image->sx, webcam->image->sy, GL_RGBA8))
				{
					allocateImage(webcam->image->sx, webcam->image->sy);
				}
				
				texture.upload(webcam->image->data, 16, webcam->image->pitch / 4, GL_RGBA, GL_UNSIGNED_BYTE);
			}
			
			imageCpuOutput.setDataRGBA8(webcam->image->data, webcam->image->sx, webcam->image->sy, 16, webcam->image->pitch);
			
			lastImageIndex = webcam->image->index;
		}
	}
}

void VfxNodeWebcam::getDescription(VfxNodeDescription & d)
{
	d.add("output image", imageOutput);
}

void VfxNodeWebcam::freeImage()
{
	lastImageIndex = -1;

	texture.free();

	imageOutput.texture = 0;
	imageCpuOutput.reset();
}

void VfxNodeWebcam::allocateImage(const int sx, const int sy)
{
	freeImage();

	texture.allocate(sx, sy, GL_RGBA8, true, true);
	
	imageOutput.texture = texture.id;
}
