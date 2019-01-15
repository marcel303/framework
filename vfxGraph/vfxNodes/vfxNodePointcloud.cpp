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

#include "vfxNodePointcloud.h"

VFX_ENUM_TYPE(pointcloudMode)
{
	elem("Kinect V1");
	elem("Kinect V2");
}

VFX_NODE_TYPE(VfxNodePointcloud)
{
	typeName = "pointcloud";
	
	in("depth", "channel");
	inEnum("mode", "pointcloudMode");
	out("x", "channel");
	out("y", "channel");
	out("z", "channel");
}

VfxNodePointcloud::VfxNodePointcloud()
	: VfxNodeBase()
	, xyzChannelData()
	, xOutput()
	, yOutput()
	, zOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DepthChannel, kVfxPlugType_Channel);
	addInput(kInput_Mode, kVfxPlugType_Int);
	addOutput(kOutput_X, kVfxPlugType_Channel, &xOutput);
	addOutput(kOutput_Y, kVfxPlugType_Channel, &yOutput);
	addOutput(kOutput_Z, kVfxPlugType_Channel, &zOutput);
}

namespace KinectV1
{
	namespace CameraParams
	{
		static const double fx_d = 1.0 / 5.9421434211923247e+02;
		static const double fy_d = 1.0 / 5.9104053696870778e+02;
		static const double cx_d = 3.3930780975300314e+02;
		static const double cy_d = 2.4273913761751615e+02;
	}
	
	static void depthToCameraXYZ(int px, int py, float depth, float & x, float & y, float & z)
	{
		z = depth / 1000.f;
		x = (float)((px - CameraParams::cx_d) * z * CameraParams::fx_d);
		y = (float)((py - CameraParams::cy_d) * z * CameraParams::fy_d);
	}
}

namespace KinectV2
{
	namespace CameraParams
	{
		static const float fx = 365.456f;
		static const float fy = 365.456f;
		static const float cx = 254.878f;
		static const float cy = 205.395f;
	}
	
	static void depthToCameraXYZ(int px, int py, float depth, float & x, float & y, float & z)
	{
		z = depth / 1000.f;
		x = (px - CameraParams::cx) * z / CameraParams::fx;
		y = (py - CameraParams::cy) * z / CameraParams::fy;
	}
}

void VfxNodePointcloud::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodePointcloud);
	
	const VfxChannel * channel = getInputChannel(kInput_DepthChannel, nullptr);
	const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
	
	if (channel == nullptr || channel->sx == 0 || channel->sy == 0)
	{
		xyzChannelData.free();
		
		xOutput.reset();
		yOutput.reset();
		zOutput.reset();
	}
	else
	{
		xyzChannelData.alloc(channel->sx * channel->sy * 3);
		
		const int pitch = channel->sx * channel->sy;
		
		xOutput.setData2D(xyzChannelData.data + pitch * 0, false, channel->sx, channel->sy);
		yOutput.setData2D(xyzChannelData.data + pitch * 1, false, channel->sx, channel->sy);
		zOutput.setData2D(xyzChannelData.data + pitch * 2, false, channel->sx, channel->sy);
		
		auto & dChannel = *channel;
		auto & xChannel = xOutput;
		auto & yChannel = yOutput;
		auto & zChannel = zOutput;
		
		const float * __restrict dItr = dChannel.data;
		      float * __restrict xItr = xChannel.dataRw();
		      float * __restrict yItr = yChannel.dataRw();
		      float * __restrict zItr = zChannel.dataRw();
		
		// convert depth values into XYZ coordinates
		
		if (mode == kMode_Kinect1)
		{
			for (int y = 0; y < channel->sy; ++y)
				for (int x = 0; x < channel->sx; ++x)
					KinectV1::depthToCameraXYZ(x, y, *dItr++, *xItr++, *yItr++, *zItr++);
		}
		else
		{
			for (int y = 0; y < channel->sy; ++y)
				for (int x = 0; x < channel->sx; ++x)
					KinectV2::depthToCameraXYZ(x, y, *dItr++, *xItr++, *yItr++, *zItr++);
		}
	}
}

void VfxNodePointcloud::getDescription(VfxNodeDescription & d)
{
	d.add("output XYZ:");
	d.add(xOutput);
	d.add(yOutput);
	d.add(zOutput);
}
