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

#if ENABLE_PS3EYE

#include "Log.h"
#include "vfxNodePs3eye.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>

// todo : add mutex and copy captured frame data

using namespace ps3eye;

VFX_NODE_TYPE(VfxNodePs3eye)
{
	typeName = "ps3eye";
	
	in("device", "int");
	out("image", "image");
	out("image_mem", "image_cpu");
}

VfxNodePs3eye::VfxNodePs3eye()
	: VfxNodeBase()
	, currentDeviceIndex(-1)
	, captureThread(nullptr)
	, stopCaptureThread(false)
	, ps3eye()
	, frameData(nullptr)
	, texture()
	, imageOutput()
	, imageCpuOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceIndex, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	addOutput(kOutput_ImageCpu, kVfxPlugType_ImageCpu, &imageCpuOutput);
}

VfxNodePs3eye::~VfxNodePs3eye()
{
	freeImage();
	
	stopCapture();
	
	ps3eye = nullptr;
}

void VfxNodePs3eye::tick(const float dt)
{
	if (isPassthrough)
	{
		freeImage();
		
		return;
	}
	
	vfxCpuTimingBlock(VfxNodePs3eye);
	vfxGpuTimingBlock(VfxNodePs3eye);
	
	const int desiredSx = 320;
	const int desiredSy = 240;
	const int desiredFps = 187;
	const bool enableColor = true;

	const int deviceIndex = getInputInt(kInput_DeviceIndex, 0);
	
	if (deviceIndex != currentDeviceIndex)
	{
		currentDeviceIndex = deviceIndex;
		
		stopCapture();

    	// list the devices

    	auto devices = PS3EYECam::getDevices();
		LOG_DBG("found %d PS3 eye cam devices", devices.size());

		if (!devices.empty())
		{
			if (deviceIndex >= 0 && deviceIndex < devices.size())
				ps3eye = devices.at(deviceIndex);
			else
				ps3eye = devices.at(0);
		}
		
		if (ps3eye != nullptr)
		{
			const bool result = ps3eye->init(desiredSx, desiredSy, desiredFps,
				enableColor
				? PS3EYECam::EOutputFormat::RGB
				: PS3EYECam::EOutputFormat::Gray);

			if (result == false)
			{
				ps3eye = nullptr;
			}
			else
			{
				const int sx = ps3eye->getWidth();
				const int sy = ps3eye->getHeight();
				
				frameData = new uint8_t[sx * sy * (enableColor ? 3 : 1)];
				
				ps3eye->start();
				
				captureThread = SDL_CreateThread(captureThreadProc, "PS3EYE Capture Thread", this);
			}
		}
	}

	if (ps3eye == nullptr)
	{
		freeImage();
	}
	else
	{
		if (true)
		{
			const bool wantsTexture = outputs[kOutput_Image].isReferenced();
			
			const int sx = ps3eye->getWidth();
			const int sy = ps3eye->getHeight();

			if (wantsTexture)
			{
				vfxGpuTimingBlock(VfxNodePs3eye);
				
				const int internalFormat = enableColor ? GL_RGB8 : GL_R8;

				if (texture.isChanged(sx, sy, internalFormat))
				{
					allocateImage(sx, sy, internalFormat);
					if (enableColor)
						texture.setSwizzle(GL_RED, GL_GREEN, GL_BLUE, GL_ONE);
					else
						texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
				}
				
				texture.upload(frameData, 1, sx, enableColor ? GL_RGB : GL_RED, GL_UNSIGNED_BYTE);
			}
			
			if (enableColor)
				imageCpuOutput.setDataRGB8(frameData, sx, sy, 1, sx * 3);
			else
				imageCpuOutput.setDataR8(frameData, sx, sy, 1, sx * 1);
		}
	}
}

void VfxNodePs3eye::getDescription(VfxNodeDescription & d)
{
	d.add("output image", imageOutput);
}

void VfxNodePs3eye::freeImage()
{
	texture.free();

	imageOutput.texture = 0;
	imageCpuOutput.reset();
}

void VfxNodePs3eye::allocateImage(const int sx, const int sy, const int internalFormat)
{
	freeImage();

	texture.allocate(sx, sy, internalFormat, true, true);
	
	imageOutput.texture = texture.id;
}

int VfxNodePs3eye::captureThreadProc(void * obj)
{
	VfxNodePs3eye * self = (VfxNodePs3eye*)obj;
	
	while (self->stopCaptureThread == false)
	{
		self->ps3eye->getFrame(self->frameData);
	}
	
	return 0;
}

void VfxNodePs3eye::stopCapture()
{
	stopCaptureThread = true;
	
	SDL_WaitThread(captureThread, nullptr);
	captureThread = nullptr;
	
	stopCaptureThread = false;
	
	if (ps3eye != nullptr && ps3eye->isStreaming())
		ps3eye->stop();

	ps3eye = nullptr;

	delete [] frameData;
	frameData = nullptr;
}

#endif
