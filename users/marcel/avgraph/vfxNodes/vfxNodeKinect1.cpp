#include "framework.h"
#include "kinect1.h"
#include "vfxNodeKinect1.h"

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
	kinect->lockBuffers();
	{
		if (kinect->hasVideo)
		{
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
