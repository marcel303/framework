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

#include "kinect1.h"

#if ENABLE_KINECT1

#include "vfxNodeKinect1.h"

#include "framework.h" // todo : use OpenGL texture object and remove this dependency

VFX_NODE_TYPE(kinect1, VfxNodeKinect1)
{
	typeName = "kinect1";
	
	in("deviceId", "int");
	in("infrared", "bool");
	out("video", "image");
	out("depth", "image");
}

VfxNodeKinect1::VfxNodeKinect1()
	: VfxNodeBase()
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
	kinect->shut();

	delete kinect;
	kinect = nullptr;	
}

void VfxNodeKinect1::init(const GraphNode & node)
{
	const bool videoIsInfrared = getInputBool(kInput_Infrared, false);
	
	kinect = new Kinect1();
	kinect->bIsVideoInfrared = videoIsInfrared;
	
	kinect->init();
}

void VfxNodeKinect1::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeKinect1);
	
	kinect->lockBuffers();
	{
		if (kinect->hasVideo)
		{
			vfxGpuTimingBlock(VfxNodeKinect1_VideoTextureUpload);
			
			kinect->hasVideo = false;
			
			// create texture from video data
			
			if (videoImage.texture != 0)
			{
				glDeleteTextures(1, &videoImage.texture);
			}
			
			if (kinect->bIsVideoInfrared)
			{
				videoImage.texture = createTextureFromR8(kinect->video, kinect->width, kinect->height, true, true);
				
				glBindTexture(GL_TEXTURE_2D, videoImage.texture);
				GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
			else
				videoImage.texture = createTextureFromRGB8(kinect->video, kinect->width, kinect->height, true, true);
		}
		
		if (kinect->hasDepth)
		{
			vfxGpuTimingBlock(VfxNodeKinect1_DepthTextureUpload);
			
			kinect->hasDepth = false;
			
			// FREENECT_DEPTH_MM_MAX_VALUE
			// FREENECT_DEPTH_MM_NO_VALUE
			
			// create texture from depth data
			
			if (depthImage.texture != 0)
			{
				glDeleteTextures(1, &depthImage.texture);
			}
			
			depthImage.texture = createTextureFromR16(kinect->depth, kinect->width, kinect->height, true, true);
			
			glBindTexture(GL_TEXTURE_2D, depthImage.texture);
			GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		}
	}
	kinect->unlockBuffers();
}

#endif
