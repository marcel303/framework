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

VFX_ENUM_TYPE(ps3eyeResolution)
{
	elem("320x240");
	//elem("640x480");
}

VFX_NODE_TYPE(VfxNodePs3eye)
{
	typeName = "ps3eye";
	
	in("device", "int");
	inEnum("resolution", "ps3eyeResolution");
	in("fps", "int", "100");
	in("color", "bool", "1");
	in("autoColors", "bool", "1");
	in("gain", "float", "0.32");
	in("exposure", "float", "0.47");
	in("balanceR", "float", "0.5");
	in("balanceG", "float", "0.5");
	in("balanceB", "float", "0.5");
	out("image", "image");
	out("image_mem", "image_cpu");
}

VfxNodePs3eye::EyeParams::EyeParams()
{
	memset(this, 0, sizeof(*this));
}

VfxNodePs3eye::VfxNodePs3eye()
	: VfxNodeBase()
	, currentDeviceIndex(-1)
	, currentResolution(0)
	, currentFramerate(0)
	, currentEnableColor(false)
	, captureThread(nullptr)
	, mutex(nullptr)
	, stopCaptureThread(false)
	, ps3eye()
	, frameData(nullptr)
	, hasFrameData(false)
	, eyeParams()
	, currentEyeParams()
	, texture()
	, imageOutput()
	, imageCpuOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceIndex, kVfxPlugType_Int);
	addInput(kInput_Resolution, kVfxPlugType_Int);
	addInput(kInput_Framerate, kVfxPlugType_Int);
	addInput(kInput_ColorEnabled, kVfxPlugType_Bool);
	addInput(kInput_AutoGain, kVfxPlugType_Bool);
	addInput(kInput_Gain, kVfxPlugType_Float);
	addInput(kInput_Exposure, kVfxPlugType_Float);
	addInput(kInput_WhiteBalanceR, kVfxPlugType_Float);
	addInput(kInput_WhiteBalanceG, kVfxPlugType_Float);
	addInput(kInput_WhiteBalanceB, kVfxPlugType_Float);
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
		
		stopCapture();
		
		currentDeviceIndex = -1;
		currentResolution = 0;
		currentFramerate = 0;
		currentEnableColor = false;
		
		return;
	}
	
	vfxCpuTimingBlock(VfxNodePs3eye);
	vfxGpuTimingBlock(VfxNodePs3eye);
	
	const int deviceIndex = getInputInt(kInput_DeviceIndex, 0);
	const Resolution resolution = (Resolution)getInputInt(kInput_Resolution, 0);
	const int desiredFramerate = getInputInt(kInput_Framerate, 100);
	const bool enableColor = getInputBool(kInput_ColorEnabled, true);
	
	SDL_LockMutex(mutex);
	{
		eyeParams.autoGain = getInputBool(kInput_AutoGain, true);
		eyeParams.gain = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_Gain, .32f) * 63.f)));
		eyeParams.exposure = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_Exposure, .47f) * 255.f)));
		eyeParams.balanceR = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_WhiteBalanceR, .5f) * 255.f)));
		eyeParams.balanceG = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_WhiteBalanceG, .5f) * 255.f)));
		eyeParams.balanceB = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_WhiteBalanceB, .5f) * 255.f)));
	}
	SDL_UnlockMutex(mutex);
	
	if (deviceIndex != currentDeviceIndex ||
		currentResolution != resolution ||
		desiredFramerate != currentFramerate ||
		enableColor != currentEnableColor)
	{
		currentDeviceIndex = deviceIndex;
		currentResolution = resolution;
		currentFramerate = desiredFramerate;
		currentEnableColor = enableColor;
		
		//
		
		stopCapture();

    	// select the device

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
			const int desiredSx = resolution == kResolution_320x240 ? 320 : 640;
			const int desiredSy = resolution == kResolution_320x240 ? 240 : 480;
	
			const bool result = ps3eye->init(
				desiredSx,
				desiredSy,
				desiredFramerate,
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
				
				const int numBytes = sx * sy * (enableColor ? 3 : 1);
				
				Assert(frameData == nullptr);
				frameData = new uint8_t[numBytes];
				Assert(hasFrameData == false);
				
				Assert(mutex == nullptr);
				mutex = SDL_CreateMutex();
				Assert(mutex != nullptr);
				
				Assert(captureThread == nullptr);
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
		if (hasFrameData)
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
	
	auto ps3eye = self->ps3eye.get();
	
	ps3eye->setAutoWhiteBalance(true);
	
	ps3eye->start();
	
	auto & currentEyeParams = self->currentEyeParams;
	
	while (self->stopCaptureThread == false)
	{
		EyeParams eyeParams;
		
		SDL_LockMutex(self->mutex);
		{
			eyeParams = self->eyeParams;
		}
		SDL_UnlockMutex(self->mutex);
		
		if (eyeParams.autoGain != currentEyeParams.autoGain || !currentEyeParams.isValid)
			ps3eye->setAutogain(eyeParams.autoGain);
		
		if (eyeParams.gain != currentEyeParams.gain || !currentEyeParams.isValid)
			ps3eye->setGain(eyeParams.gain);
		
		if (eyeParams.exposure != currentEyeParams.exposure || !currentEyeParams.isValid)
			ps3eye->setExposure(eyeParams.exposure);
		
		if (eyeParams.balanceR != currentEyeParams.balanceR)
			ps3eye->setRedBalance(eyeParams.balanceR);
		if (eyeParams.balanceG != currentEyeParams.balanceG)
			ps3eye->setGreenBalance(eyeParams.balanceG);
		if (eyeParams.balanceB != currentEyeParams.balanceB)
			ps3eye->setBlueBalance(eyeParams.balanceB);
		
		currentEyeParams = eyeParams;
		currentEyeParams.isValid = true;
		
		//
		
		ps3eye->getFrame(self->frameData);
		
		self->hasFrameData = true;
	}
	
	currentEyeParams = EyeParams();
	
	ps3eye->stop();
	
	return 0;
}

void VfxNodePs3eye::stopCapture()
{
	if (captureThread != nullptr)
	{
		stopCaptureThread = true;
		
		SDL_WaitThread(captureThread, nullptr);
		captureThread = nullptr;
		
		stopCaptureThread = false;
	}
	
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	
	if (ps3eye != nullptr)
	{
		Assert(!ps3eye->isStreaming());

		ps3eye = nullptr;
	}

	delete [] frameData;
	frameData = nullptr;
	
	hasFrameData = false;
}

#endif
