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
static libfreenect2::Frame * depthFrame = nullptr;
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
	addOutput(kOutput_DepthChannels, kVfxPlugType_Channels, &depthChannels);
}

VfxNodeKinect2::~VfxNodeKinect2()
{
	initCount--;
	
	if (initCount == 0)
	{
		delete videoFrame;
		videoFrame = nullptr;
		
		delete depthFrame;
		depthFrame = nullptr;
		
		videoTexture.free();
		depthTexture.free();
		
		videoImageCpu.reset();
		depthChannels.reset();
		
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
	
	// todo : this check is no longer sufficient : need to also check CPU image and depth channels output
	//        BUT if only CPU image and/or depth channels is referenced, we aren't interested in making textures
	//        UNLESS! there is another node that has video and/or depth image references
	//        so we should let the first node that wants textures to update textures, if they're not up to date yet
	
	const bool wantsVideo = outputs[kOutput_VideoImage].isReferenced();
	const bool wantsDepth = outputs[kOutput_DepthImage].isReferenced();
	
	if (kinect->isInit == false || (wantsVideo == false && wantsDepth == false))
	{
		videoImage.texture = 0;
		depthImage.texture = 0;
		videoImageCpu.reset();
		depthChannels.reset();
		
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
			
			delete depthFrame;
			depthFrame = kinect->listener->depth;
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
	
	if (depthFrame != nullptr)
		depthChannels.setData2DContiguous((float*)depthFrame->data, false, depthFrame->width, depthFrame->height, 1);
	else
		depthChannels.reset();
}

void VfxNodeKinect2::getDescription(VfxNodeDescription & d)
{
	if (kinect != nullptr)
	{
		d.add("Kinect2 initialized: %d", kinect->isInit);
		d.add("capture image size: %d x %d", kinect->width, kinect->height);
		d.newline();
	}
	
	d.addOpenglTexture("video texture", videoTexture.id);
	d.newline();
	
	d.addOpenglTexture("depth texture", depthTexture.id);
	d.newline();
	
	d.add("video image", videoImageCpu);
}
