#include "kinect2.h"
#include "vfxNodeKinect2.h"
#include <libfreenect2/libfreenect2.hpp>

#include <libfreenect2/frame_listener_impl.h> // fixme : remove

VfxNodeKinect2::VfxNodeKinect2()
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

VfxNodeKinect2::~VfxNodeKinect2()
{
	kinect->shut();

	delete kinect;
	kinect = nullptr;	
}

void VfxNodeKinect2::init(const GraphNode & node)
{
	const bool videoIsInfrared = getInputBool(kInput_Infrared, false);
	
	kinect = new Kinect2();
	//kinect->bIsVideoInfrared = videoIsInfrared;
	
	kinect->init();
}

void VfxNodeKinect2::tick(const float dt)
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
			
			//if (kinect->bIsVideoInfrared)
			if (false)
			{
				videoImage.texture = createTextureFromR8(kinect->video, kinect->width, kinect->height, true, true);
				
				glBindTexture(GL_TEXTURE_2D, videoImage.texture);
				GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
			else
				videoImage.texture = createTextureFromRGB8(kinect->video->data, kinect->video->width, kinect->video->height, true, true);
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
			
			depthImage.texture = createTextureFromR32F(kinect->depth->data, kinect->depth->width, kinect->depth->height, true, true);
			
			glBindTexture(GL_TEXTURE_2D, depthImage.texture);
			GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		}
		
		// fixme : remove. kinect2 should be able to handle double buffering for efficient threading.. !
		if (kinect->frameMap != nullptr)
		{
			kinect->listener->release(*(libfreenect2::FrameMap*)kinect->frameMap);
			kinect->frameMap = nullptr;
		}
	}
	kinect->unlockBuffers();
}
