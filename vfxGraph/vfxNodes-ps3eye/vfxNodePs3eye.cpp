/*
	Copyright (C) 2020 Marcel Smit
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

#if VFXGRAPH_ENABLE_PS3EYE

#include "vfxNodePs3eye.h"

#include "Log.h"
#include "Multicore/ThreadName.h"

#include <algorithm>
#include <math.h>

// todo : add mutex and copy captured frame data

using namespace ps3eye;

VFX_ENUM_TYPE(ps3eyeResolution)
{
	elem("320x240");
	elem("640x480");
}

VFX_NODE_TYPE(VfxNodePs3eye)
{
	typeName = "ps3eye";
	
	in("device", "int");
	inEnum("resolution", "ps3eyeResolution");
	in("fps", "int", "60");
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
	, captureThread()
	, mutex()
	, stopCaptureThread(false)
	, ps3eye()
	, frameData(nullptr)
	, hasFrameData(false)
	, eyeParams()
	, currentEyeParams()
	, texture()
	, imageOutput()
	, imageCpuData()
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
	
	Assert(ps3eye == nullptr);
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
	const int desiredFramerate = std::min(
		resolution == kResolution_640x480
			? 60
			: 180,
		getInputInt(kInput_Framerate, 100));
	const bool enableColor = getInputBool(kInput_ColorEnabled, true);
	
	mutex.lock();
	{
		eyeParams.autoGain = getInputBool(kInput_AutoGain, true);
		eyeParams.gain = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_Gain, .32f) * 63.f)));
		eyeParams.exposure = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_Exposure, .47f) * 255.f)));
		eyeParams.balanceR = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_WhiteBalanceR, .5f) * 255.f)));
		eyeParams.balanceG = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_WhiteBalanceG, .5f) * 255.f)));
		eyeParams.balanceB = std::max(0, std::min(255, (int)roundf(getInputFloat(kInput_WhiteBalanceB, .5f) * 255.f)));
	}
	mutex.unlock();
	
	if (deviceIndex != currentDeviceIndex ||
		resolution != currentResolution ||
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
				? PS3EYECam::EOutputFormat::RGBA
				: PS3EYECam::EOutputFormat::Gray);

			if (result == false)
			{
				ps3eye = nullptr;
			}
			else
			{
				const int sx = ps3eye->getWidth();
				const int sy = ps3eye->getHeight();
				
				const int numBytes = sx * sy * (enableColor ? 4 : 1);
				
				Assert(frameData == nullptr);
				frameData = new uint8_t[numBytes];
				Assert(hasFrameData == false);
				
				captureThread = std::thread(captureThreadProc, this);
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
				
				const GX_TEXTURE_FORMAT format = enableColor ? GX_RGBA8_UNORM : GX_R8_UNORM;

				if (texture.isChanged(sx, sy, format))
				{
					allocateImage(sx, sy, format);
					if (enableColor)
						texture.setSwizzle(0, 1, 2, GX_SWIZZLE_ONE);
					else
						texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
				}
				
				texture.upload(frameData, 1, sx);
			}
			
			if (enableColor)
			{
				imageCpuData.allocOnSizeChange(sx, sy, 4);
				imageCpuOutput = imageCpuData.image;
				
				VfxImageCpu::deinterleave4(
					frameData,
					sx, sy, 4, sx * 4,
					imageCpuOutput.channel[0],
					imageCpuOutput.channel[1],
					imageCpuOutput.channel[2],
					imageCpuOutput.channel[3]);
			}
			else
			{
				imageCpuData.free();
				
				imageCpuOutput.setDataR8(frameData, sx, sy, 1, sx);
			}
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
	imageCpuData.free();
	imageCpuOutput.reset();
}

void VfxNodePs3eye::allocateImage(const int sx, const int sy, const GX_TEXTURE_FORMAT format)
{
	freeImage();

	texture.allocate(sx, sy, format, true, true);
	
	imageOutput.texture = texture.id;
}

void VfxNodePs3eye::captureThreadProc(VfxNodePs3eye * self)
{
	SetCurrentThreadName("VfxNodePs3eye");
	
	auto ps3eye = self->ps3eye.get();
	
	ps3eye->setAutoWhiteBalance(true);
	
	ps3eye->start();
	
	auto & currentEyeParams = self->currentEyeParams;
	
	while (self->stopCaptureThread == false)
	{
		EyeParams eyeParams;
		
		self->mutex.lock();
		{
			eyeParams = self->eyeParams;
		}
		self->mutex.unlock();
		
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
}

void VfxNodePs3eye::stopCapture()
{
	if (captureThread.joinable())
	{
		stopCaptureThread = true;
		{
			captureThread.join();
		}
		stopCaptureThread = false;
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

void linkVfxNodes_Ps3eye()
{
}
