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

#include "kinect1.h"

#if ENABLE_KINECT1

#include "vfxNodeKinect1.h"

VFX_NODE_TYPE(VfxNodeKinect1)
{
	typeName = "kinect1";
	
	in("deviceId", "int");
	in("infrared", "bool");
	out("video", "image");
	out("depth", "image");
}

VfxNodeKinect1::VfxNodeKinect1()
	: VfxNodeBase()
	, videoTexture()
	, depthTexture()
	, videoImage()
	, depthImage()
	, kinect(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceId, kVfxPlugType_Int);
	addInput(kInput_Infrared, kVfxPlugType_Bool);
	addOutput(kOutput_VideoImage, kVfxPlugType_Image, &videoImage);
	addOutput(kOutput_DepthImage, kVfxPlugType_Image, &depthImage);
}

VfxNodeKinect1::~VfxNodeKinect1()
{
	videoTexture.free();
	depthTexture.free();
	
	videoImage.reset();
	depthImage.reset();
	
	if (kinect != nullptr)
	{
		kinect->shut();

		delete kinect;
		kinect = nullptr;
	}
}

void VfxNodeKinect1::init(const GraphNode & node)
{
	if (isPassthrough)
		return;
	
	const bool videoIsInfrared = getInputBool(kInput_Infrared, false);

	kinect = new Kinect1();
	kinect->bIsVideoInfrared = videoIsInfrared;
	
	kinect->init();
}

void VfxNodeKinect1::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeKinect1);
	
	if (isPassthrough)
	{
		videoTexture.free();
		depthTexture.free();
		
		videoImage.reset();
		depthImage.reset();
		
		if (kinect != nullptr)
		{
			kinect->shut();
			
			delete kinect;
			kinect = nullptr;
		}
		
		return;
	}
	
	// (re)create the kinect device if necessary
	
	if (kinect == nullptr)
	{
		kinect = new Kinect1();
		kinect->bIsVideoInfrared = videoIsInfrared;
		
		kinect->init();
	}
	
	// check for configuration changes
	
	const bool videoIsInfrared = getInputBool(kInput_Infrared, false);
	
	if (videoIsInfrared != kinect->bIsVideoInfrared)
	{
		kinect->shut();
	
		kinect->bIsVideoInfrared = videoIsInfrared;
		kinect->init();
	}
	
	kinect->lockBuffers();
	{
		if (kinect->hasVideo)
		{
			vfxGpuTimingBlock(VfxNodeKinect1_VideoTextureUpload);
			
			kinect->hasVideo = false;
			
			// create texture from video data
			
			if (kinect->bIsVideoInfrared)
			{
				if (videoTexture.isChanged(kinect->width, kinect->height, GX_R8_UNORM))
				{
					videoTexture.alloc(kinect->width, kinect->height, GX_R8_UNORM, true, true);
					videoTexture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
				}
				
				videoTexture.upload(kinect->video);
				
				videoImage.texture = videoTexture.id;
			}
			else
			{
				if (videoTexture.isChanged(kinect->width, kinect->height, GX_RGB8_UNORM))
				{
					videoTexture.alloc(kinect->width, kinect->height, GX_RGB8_UNORM, true, true);
					videoTexture.setSwizzle(0, 1, 2, GX_SWIZZLE_ONE);
				}
				
				videoTexture.upload(kinect->video);
				
				videoImage.texture = videoTexture.id;
			}
		}
		
		if (kinect->hasDepth)
		{
			vfxGpuTimingBlock(VfxNodeKinect1_DepthTextureUpload);
			
			kinect->hasDepth = false;
			
			// FREENECT_DEPTH_MM_MAX_VALUE
			// FREENECT_DEPTH_MM_NO_VALUE
			
			// create texture from depth data
			
			if (depthTexture.isChanged(kinect->width, kinect->height, GX_R16_UNORM))
			{
				depthTexture.alloc(kinect->width, kinect->height, GX_R16_UNORM, true, true);
				depthTexture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
			}
		
			depthTexture.upload(kinect->depth);
			
			depthImage.texture = depthTexture.id;
		}
	}
	kinect->unlockBuffers();
}

#endif
