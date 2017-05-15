#include "kinect2.h"
#include "kinect2FrameListener.h"
#include "vfxNodeKinect2.h"
#include <libfreenect2/libfreenect2.hpp>

#include <libfreenect2/frame_listener_impl.h> // fixme : remove

static int initCount = 0;
static Kinect2 * kinect = nullptr;
static GLuint videoTexture = 0;
static GLuint depthTexture = 0;

VfxNodeKinect2::VfxNodeKinect2()
	: VfxNodeBase()
	, videoImage()
	, depthImage()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceId, kVfxPlugType_Int);
	addInput(kInput_Infrared, kVfxPlugType_Bool);
	addOutput(kOutput_VideoImage, kVfxPlugType_Image, &videoImage);
	addOutput(kOutput_DepthImage, kVfxPlugType_Image, &depthImage);
}

VfxNodeKinect2::~VfxNodeKinect2()
{
	initCount--;
	
	if (initCount == 0)
	{
		if (videoTexture != 0)
		{
			glDeleteTextures(1, &videoTexture);
			videoTexture = 0;
		}
		
		if (depthTexture != 0)
		{
			glDeleteTextures(1, &depthTexture);
			depthTexture = 0;
		}
		
		//
		
		kinect->shut();
		
		delete kinect;
		kinect = nullptr;
	}
}

void VfxNodeKinect2::init(const GraphNode & node)
{
	if (initCount == 0)
	{
		Assert(videoTexture == 0);
		Assert(depthTexture == 0);
		
		kinect = new Kinect2();
		
		kinect->init();
	}
	
	initCount++;
}

void VfxNodeKinect2::tick(const float dt)
{
	kinect->listener->lockBuffers();
	{
		if (kinect->listener->video)
		{
			// create texture from video data
			
			if (videoTexture != 0)
			{
				glDeleteTextures(1, &videoTexture);
				videoTexture = 0;
			}
			
			videoTexture = createTextureFromRGBA8(kinect->listener->video->data, kinect->listener->video->width, kinect->listener->video->height, true, true);
			
			glBindTexture(GL_TEXTURE_2D, videoTexture);
			GLint swizzleMask[4] = { GL_BLUE, GL_GREEN, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			
			// consume video data
			
			delete kinect->listener->video;
			kinect->listener->video = nullptr;
		}
		
		if (kinect->listener->depth)
		{
			// create texture from depth data
			
			if (depthTexture != 0)
			{
				glDeleteTextures(1, &depthTexture);
				depthTexture = 0;
			}
			
			depthTexture = createTextureFromR32F(kinect->listener->depth->data, kinect->listener->depth->width, kinect->listener->depth->height, true, true);
			
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			
			// consume depth data
			
			delete kinect->listener->depth;
			kinect->listener->depth = nullptr;
		}
	}
	kinect->listener->unlockBuffers();
	
	videoImage.texture = videoTexture;
	depthImage.texture = depthTexture;
}
