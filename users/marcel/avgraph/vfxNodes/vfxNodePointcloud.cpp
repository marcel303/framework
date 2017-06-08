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

VfxNodePointcloud::VfxNodePointcloud()
	: VfxNodeBase()
	, xyzChannelData()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DepthChannel, kVfxPlugType_Channels);
	addInput(kInput_Mode, kVfxPlugType_Int);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &xyzOutput);
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
		x = (px - CameraParams::cx) * depth / CameraParams::fx;
		y = (py - CameraParams::cy) * depth / CameraParams::fy;
	}
}

void VfxNodePointcloud::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodePointcloud);
	
	const VfxChannels * channels = getInputChannels(kInput_DepthChannel, nullptr);
	const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
	
	if (channels == nullptr || channels->sx == 0 || channels->sy == 0 || channels->numChannels == 0)
	{
		xyzChannelData.free();
		
		xyzOutput.reset();
	}
	else
	{
		xyzChannelData.alloc(channels->sx * channels->sy * 3);
		
		xyzOutput.setData2DContiguous(xyzChannelData.data, false, channels->sx, channels->sy, 3);
		
		const auto & dChannel = channels->channels[0];
		      auto & xChannel = xyzOutput.channels[0];
		      auto & yChannel = xyzOutput.channels[1];
		      auto & zChannel = xyzOutput.channels[2];
		
		const float * __restrict dItr = dChannel.data;
		      float * __restrict xItr = xChannel.dataRw();
		      float * __restrict yItr = yChannel.dataRw();
		      float * __restrict zItr = zChannel.dataRw();
		
		// todo : convert depth values into XYZ coordinates
		
		if (mode == kMode_Kinect1)
		{
			for (int y = 0; y < channels->sy; ++y)
				for (int x = 0; x < channels->sx; ++x)
					KinectV1::depthToCameraXYZ(x, y, *dItr++, *xItr++, *yItr++, *zItr++);
		}
		else
		{
			for (int y = 0; y < channels->sy; ++y)
				for (int x = 0; x < channels->sx; ++x)
					KinectV2::depthToCameraXYZ(x, y, *dItr++, *xItr++, *yItr++, *zItr++);
		}
	}
}

void VfxNodePointcloud::getDescription(VfxNodeDescription & d)
{
	d.add("output XYZ:");
	d.add(xyzOutput);
}
