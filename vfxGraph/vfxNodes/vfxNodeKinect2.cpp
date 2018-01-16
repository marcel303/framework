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

#include "kinect2.h"

#if ENABLE_KINECT2

#include "kinect2FrameListener.h"
#include "openglTexture.h"
#include "vfxNodeKinect2.h"
#include <GL/glew.h> // GL_RED
#include <libfreenect2/libfreenect2.hpp>

// kinect device and video frames/textures. note that these are all shared between kinect2 nodes, as
// it would be senseless for each node to compute these over and over again with the same data coming
// from the kinect. share them globally so we only need to compute them once for each new frame

static int initCount = 0;

static Kinect2 * kinect = nullptr;
static libfreenect2::Frame * videoFrame = nullptr;
static libfreenect2::Frame * depthFrame = nullptr;
static OpenglTexture videoTexture;
static OpenglTexture depthTexture;
static int tickId = 0;
static int videoFrameTickId = 0;
static int depthFrameTickId = 0;
static int videoTextureTickId = 0;
static int depthTextureTickId = 0;

//

VFX_NODE_TYPE(VfxNodeKinect2)
{
	typeName = "kinect2";
	
	in("deviceId", "int");
	in("infrared", "bool");
	out("video", "image");
	out("depth", "image");
	out("mem_video", "image_cpu");
	out("ch_depth", "channel");
}

VfxNodeKinect2::VfxNodeKinect2()
	: VfxNodeBase()
	, videoImage()
	, depthImage()
	, videoImageCpu()
	, depthChannel()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DeviceId, kVfxPlugType_Int);
	addInput(kInput_Infrared, kVfxPlugType_Bool);
	addOutput(kOutput_VideoImage, kVfxPlugType_Image, &videoImage);
	addOutput(kOutput_DepthImage, kVfxPlugType_Image, &depthImage);
	addOutput(kOutput_VideoImageCpu, kVfxPlugType_ImageCpu, &videoImageCpu);
	addOutput(kOutput_DepthChannel, kVfxPlugType_Channel, &depthChannel);
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
	
	if (isPassthrough || kinect->isInit == false)
	{
		videoImage.texture = 0;
		depthImage.texture = 0;
		
		videoImageCpu.reset();
		depthChannel.reset();
		
		return;
	}
	
	const bool wantsVideoTexture = outputs[kOutput_VideoImage].isReferenced();
	const bool wantsDepthTexture = outputs[kOutput_DepthImage].isReferenced();
	const bool wantsVideoData = outputs[kOutput_VideoImageCpu].isReferenced();
	const bool wantsDepthData = outputs[kOutput_DepthChannel].isReferenced();
	
	tickId++;
	
	libfreenect2::Frame * videoFrameToFree = nullptr;
	libfreenect2::Frame * depthFrameToFree = nullptr;
	
	kinect->listener->lockBuffers();
	{
		if (kinect->listener->video && (wantsVideoTexture || wantsVideoData))
		{
			// consume video data
			
			videoFrameToFree = videoFrame;
			videoFrame = kinect->listener->video;
			kinect->listener->video = nullptr;
			
			videoFrameTickId = tickId;
		}
		
		if (kinect->listener->depth && (wantsDepthTexture || wantsDepthData))
		{
			// consume depth data
			
			depthFrameToFree = depthFrame;
			depthFrame = kinect->listener->depth;
			kinect->listener->depth = nullptr;
			
			depthFrameTickId = tickId;
		}
	}
	kinect->listener->unlockBuffers();
	
	// free data outside of the lock so we don't block the Kinect processing too much
	
	delete videoFrameToFree;
	videoFrameToFree = nullptr;
	
	delete depthFrameToFree;
	depthFrameToFree = nullptr;
	
	// has the video texture been updated ?
	
	if (wantsVideoTexture && videoTextureTickId != videoFrameTickId)
	{
		vfxGpuTimingBlock(VfxNodeKinect2_VideoTextureUpload);
		
		// create texture from video data
		
		if (videoTexture.isChanged(videoFrame->width, videoFrame->height, GL_RGBA8))
		{
			videoTexture.allocate(videoFrame->width, videoFrame->height, GL_RGBA8, true, true);
			videoTexture.setSwizzle(GL_BLUE, GL_GREEN, GL_RED, GL_ONE);
		}
		
		videoTexture.upload(videoFrame->data, 16, videoFrame->width, GL_RGBA, GL_UNSIGNED_BYTE);
		
		videoTextureTickId = videoFrameTickId;
	}
	
	if (wantsDepthTexture && depthTextureTickId != depthFrameTickId)
	{
		vfxGpuTimingBlock(VfxNodeKinect2_DepthTextureUpload);
		
		// create texture from depth data
		
		if (depthTexture.isChanged(depthFrame->width, depthFrame->height, GL_R32F))
		{
			depthTexture.allocate(depthFrame->width, depthFrame->height, GL_R32F, true, true);
			depthTexture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
		}
		
		depthTexture.upload(depthFrame->data, 16, depthFrame->width, GL_RED, GL_FLOAT);
		
		depthTextureTickId = depthFrameTickId;
	}
	
	// update outputs
	
	if (wantsVideoTexture)
		videoImage.texture = videoTexture.id;
	else
		videoImage.texture = 0;
	
	if (wantsDepthTexture)
		depthImage.texture = depthTexture.id;
	else
		depthImage.texture = 0;
	
	if (videoFrame != nullptr)
		videoImageCpu.setDataRGBA8(videoFrame->data, videoFrame->width, videoFrame->height, 16, videoFrame->width * 4);
	else
		videoImageCpu.reset();
	
	if (depthFrame != nullptr)
		depthChannel.setData2D((float*)depthFrame->data, false, depthFrame->width, depthFrame->height);
	else
		depthChannel.reset();
}

void VfxNodeKinect2::getDescription(VfxNodeDescription & d)
{
	if (kinect != nullptr)
	{
		d.add("Kinect2 initialized: %d", kinect->isInit);
		if (videoTexture.id != 0)
			d.add("video capture size: %d x %d", videoTexture.sx, videoTexture.sy);
		if (depthTexture.id != 0)
			d.add("depth capture size: %d x %d", depthTexture.sx, depthTexture.sy);
		d.newline();
	}
	
	d.addOpenglTexture("video texture", videoTexture.id);
	d.newline();
	
	d.addOpenglTexture("depth texture", depthTexture.id);
	d.newline();
	
	d.add("video image", videoImageCpu);
	
	d.add("depth channel", depthChannel);
}

#endif
