#include "kinect2.h"
#include "kinect2FrameListener.h"
#include "openglTexture.h"
#include "vfxNodeKinect2.h"
#include <GL/glew.h>
#include <libfreenect2/libfreenect2.hpp>

#include <libfreenect2/frame_listener_impl.h> // fixme : remove

// kinect device and video frames/textures. note that these are all shared between kinect2 nodes, as
// it would be senseless for each node to compute these over and over again with the same data coming
// from the kinect. share them globally so we only need to compute them once for each new frame

static int initCount = 0;

static Kinect2 * kinect = nullptr;
static libfreenect2::Frame * videoFrame = nullptr;
static OpenglTexture videoTexture;
static OpenglTexture depthTexture;

//

VfxNodeKinect2::VfxNodeKinect2()
	: VfxNodeBase()
	, videoImage()
	, depthImage()
	, videoImageCpu()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceId, kVfxPlugType_Int);
	addInput(kInput_Infrared, kVfxPlugType_Bool);
	addOutput(kOutput_VideoImage, kVfxPlugType_Image, &videoImage);
	addOutput(kOutput_DepthImage, kVfxPlugType_Image, &depthImage);
	addOutput(kOutput_VideoImageCpu, kVfxPlugType_ImageCpu, &videoImageCpu);
}

VfxNodeKinect2::~VfxNodeKinect2()
{
	initCount--;
	
	if (initCount == 0)
	{
		delete videoFrame;
		videoFrame = nullptr;
		
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
		Assert(videoFrame == nullptr);
		
		Assert(videoTexture.id == 0);
		Assert(depthTexture.id == 0);
		
		Assert(kinect == nullptr);
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
	vfxCpuTimingBlock(VfxNodeKinect2);
	
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
			vfxGpuTimingBlock(VfxNodeKinect2_VideoTextureUpload);
			
			// create texture from video data
			
			if (videoTexture.isChanged(kinect->listener->video->width, kinect->listener->video->height, GL_RGBA8))
			{
				videoTexture.allocate(kinect->listener->video->width, kinect->listener->video->height, GL_RGBA8, true, true);
				videoTexture.setSwizzle(GL_BLUE, GL_GREEN, GL_RED, GL_ONE);
			}
			
			videoTexture.upload(kinect->listener->video->data, 16, kinect->listener->video->width * 4, GL_RGBA, GL_UNSIGNED_BYTE);
			
			// consume video data
			
			delete videoFrame;
			videoFrame = kinect->listener->video;
			kinect->listener->video = nullptr;
		}
		
		if (kinect->listener->depth && wantsDepth)
		{
			vfxGpuTimingBlock(VfxNodeKinect2_DepthTextureUpload);
			
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
	
	// update outputs
	
	videoImage.texture = videoTexture.id;
	depthImage.texture = depthTexture.id;
	
	if (videoFrame != nullptr)
		videoImageCpu.setDataRGBA8(videoFrame->data, videoFrame->width, videoFrame->height, 16, videoFrame->width * 4);
	else
		videoImageCpu.reset();
}

void VfxNodeKinect2::getDescription(VfxNodeDescription & d)
{
	if (kinect != nullptr)
	{
		d.add("Kinect2 initialized: %d", kinect->isInit);
		d.add("capture image size: %d x %d", kinect->width, kinect->height);
		d.newline();
	}
	
	d.add("video OpenGL texture:");
	d.addOpenglTexture(videoTexture.id);
	d.newline();
	
	d.add("depth OpenGL texture:");
	d.addOpenglTexture(depthTexture.id);
	d.newline();
	
	d.add("video image", videoImageCpu);
}
