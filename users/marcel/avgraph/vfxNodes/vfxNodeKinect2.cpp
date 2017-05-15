#include "kinect2.h"
#include "kinect2FrameListener.h"
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
	kinect->listener->lockBuffers();
	{
		if (kinect->listener->video)
		{
			// create texture from video data
			
			if (videoImage.texture != 0)
			{
				glDeleteTextures(1, &videoImage.texture);
			}
			
			//if (kinect->bIsVideoInfrared)
			if (false)
			{
				videoImage.texture = createTextureFromR8(kinect->listener->video, kinect->listener->video->width, kinect->listener->video->height, true, true);
				
				glBindTexture(GL_TEXTURE_2D, videoImage.texture);
				GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
			else
				videoImage.texture = createTextureFromRGB8(kinect->listener->video->data, kinect->listener->video->width, kinect->listener->video->height, true, true);
			
			// consume video data
			
			delete kinect->listener->video;
			kinect->listener->video = nullptr;
		}
		
		if (kinect->listener->depth)
		{
			// create texture from depth data
			
			if (depthImage.texture != 0)
			{
				glDeleteTextures(1, &depthImage.texture);
			}
			
			depthImage.texture = createTextureFromR32F(kinect->listener->depth->data, kinect->listener->depth->width, kinect->listener->depth->height, true, true);
			
			glBindTexture(GL_TEXTURE_2D, depthImage.texture);
			GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			
			// consume depth data
			
			delete kinect->listener->depth;
			kinect->listener->depth = nullptr;
		}
	}
	kinect->listener->unlockBuffers();
}
