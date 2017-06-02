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

void VfxNodePointcloud::tick(const float dt)
{
	const VfxChannels * channels = getInputChannels(kInput_DepthChannel, nullptr);
	
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
		
		for (int y = 0; y < channels->sy; ++y)
		{
			for (int x = 0; x < channels->sx; ++x)
			{
				*xItr++ = x * 64;
				*yItr++ = y * 64;
				*zItr++ = *dItr;
				
				dItr++;
			}
		}
	}
}

void VfxNodePointcloud::getDescription(VfxNodeDescription & d)
{
	d.add("output XYZ:");
	d.add(xyzOutput);
}
