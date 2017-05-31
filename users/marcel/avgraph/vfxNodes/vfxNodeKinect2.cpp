#include "framework.h"
#include "kinect2.h"
#include "kinect2FrameListener.h"
#include "openglTexture.h"
#include "vfxNodeKinect2.h"
#include <libfreenect2/libfreenect2.hpp>

#include <libfreenect2/frame_listener_impl.h> // fixme : remove

static int initCount = 0;
static Kinect2 * kinect = nullptr;
static OpenglTexture videoTexture;
static OpenglTexture depthTexture;

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
		videoTexture.free();
		depthTexture.free();
		
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
		Assert(videoTexture.id == 0);
		Assert(depthTexture.id == 0);
		
		kinect = new Kinect2();
		
		if (kinect->init() == false)
		{
			kinect->shut();
		}
	}
	
	initCount++;
}

void VfxNodeKinect2::tick(const float dt)
{
	const bool wantsVideo = outputs[kOutput_VideoImage].isReferenced();
	const bool wantsDepth = outputs[kOutput_DepthImage].isReferenced();
	
	if (kinect->isInit == false || (wantsVideo == false && wantsDepth == false))
	{
		videoImage.texture = 0;
		depthImage.texture = 0;
		
		return;
	}
	
	kinect->listener->lockBuffers();
	{
		if (kinect->listener->video && wantsVideo)
		{
			// create texture from video data
			
			if (videoTexture.isChanged(kinect->listener->video->width, kinect->listener->video->height, GL_RGBA8))
			{
				videoTexture.allocate(kinect->listener->video->width, kinect->listener->video->height, GL_RGBA8, true, true);
				videoTexture.setSwizzle(GL_BLUE, GL_GREEN, GL_RED, GL_ONE);
			}
			
			videoTexture.upload(kinect->listener->video->data, 16, kinect->listener->video->width * 4, GL_RGBA, GL_UNSIGNED_BYTE);
			
			// consume video data
			
			delete kinect->listener->video;
			kinect->listener->video = nullptr;
		}
		
		if (kinect->listener->depth && wantsDepth)
		{
			// create texture from depth data
			
			if (depthTexture.isChanged(kinect->listener->depth->width, kinect->listener->depth->height, GL_R32F))
			{
				depthTexture.allocate(kinect->listener->depth->width, kinect->listener->depth->height, GL_R32F, true, true);
				depthTexture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
			}
			
			depthTexture.upload(kinect->listener->depth->data, 16, kinect->listener->depth->width * 4, GL_RED, GL_FLOAT);
			
			// consume depth data
			
			delete kinect->listener->depth;
			kinect->listener->depth = nullptr;
		}
	}
	kinect->listener->unlockBuffers();
	
	videoImage.texture = videoTexture.id;
	depthImage.texture = depthTexture.id;
}

void VfxNodeKinect2::getDescription(VfxNodeDescription & d)
{
	if (kinect != nullptr)
	{
		d.add("Kinect2 initialized: %d", kinect->isInit);
		d.add("capture image size: %d x %d", kinect->width, kinect->height);
		d.add("video OpenGL texture:");
		d.addOpenglTexture(videoTexture.id);
		d.add("depth OpenGL texture:");
		d.addOpenglTexture(depthTexture.id);
	}
}
