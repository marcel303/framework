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

#pragma once

#if ENABLE_PS3EYE

#include "openglTexture.h"
#include "ps3eye.h"
#include "vfxNodeBase.h"
#include <atomic>

struct SDL_mutex;
struct SDL_Thread;

struct VfxNodePs3eye : VfxNodeBase
{
	enum Resolution
	{
		kResolution_320x240,
		kResolution_640x480
	};
	
	enum Input
	{
		kInput_DeviceIndex,
		kInput_Resolution,
		kInput_Framerate,
		kInput_ColorEnabled,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Image,
		kOutput_ImageCpu,
		kOutput_COUNT
	};

	int currentDeviceIndex;
	int currentResolution;
	int currentFramerate;
	bool currentEnableColor;
	
	SDL_Thread * captureThread;
	std::atomic<bool> stopCaptureThread;
	ps3eye::PS3EYECam::PS3EYERef ps3eye;
	uint8_t * frameData;
	std::atomic<bool> hasFrameData;

	OpenglTexture texture;

	VfxImage_Texture imageOutput;
	VfxImageCpu imageCpuOutput;

	VfxNodePs3eye();
	~VfxNodePs3eye();
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void freeImage();
	void allocateImage(const int sx, const int sy, const int internalFormat);
	
	static int captureThreadProc(void * obj);
	void stopCapture();
};

#endif
