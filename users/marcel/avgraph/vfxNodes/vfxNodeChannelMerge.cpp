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

#include "framework.h"
#include "vfxNodeChannelMerge.h"
#include "vfxTypes.h"

VFX_ENUM_TYPE(channelMergeMergeMode)
{
	elem("append");
	elem("concat");
}

VFX_ENUM_TYPE(channelMergeWrapMode)
{
	elem("clamp");
	elem("cycle");
	elem("padZero");
}

VFX_NODE_TYPE(channels_merge, VfxNodeChannelMerge)
{
	typeName = "channels.merge";
	inEnum("mergeMode", "channelMergeMergeMode");
	inEnum("wrapMode", "channelMergeWrapMode");
	in("channels1", "channels");
	in("channels2", "channels");
	in("channels3", "channels");
	in("channels4", "channels");
	in("swizzle", "string");
	out("channels", "channels");
}

VfxNodeChannelMerge::VfxNodeChannelMerge()
	: VfxNodeBase()
	, channelData()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_MergeMode, kVfxPlugType_Int);
	addInput(kInput_WrapMode, kVfxPlugType_Int);
	addInput(kInput_Channels1, kVfxPlugType_Channels);
	addInput(kInput_Channels2, kVfxPlugType_Channels);
	addInput(kInput_Channels3, kVfxPlugType_Channels);
	addInput(kInput_Channels4, kVfxPlugType_Channels);
	addInput(kInput_Swizzle, kVfxPlugType_String);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

void VfxNodeChannelMerge::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelMerge);
	
	if (isPassthrough)
	{
		channelData.free();
		channelsOutput.reset();
		return;
	}
	
	const MergeMode mergeMode = (MergeMode)getInputInt(kInput_MergeMode, 0);
	const WrapMode wrapMode = (WrapMode)getInputInt(kInput_WrapMode, 0);
	const VfxChannels * channels1 = getInputChannels(kInput_Channels1, nullptr);
	const VfxChannels * channels2 = getInputChannels(kInput_Channels2, nullptr);
	const VfxChannels * channels3 = getInputChannels(kInput_Channels3, nullptr);
	const VfxChannels * channels4 = getInputChannels(kInput_Channels4, nullptr);
	const char * swizzleText = getInputString(kInput_Swizzle, nullptr);

	const VfxChannels * channels[4] =
	{
		channels1,
		channels2,
		channels3,
		channels4
	};
	
	channelsOutput.reset();
	
	if (swizzleText == nullptr)
	{
		channelData.free();
	}
	else
	{
		VfxSwizzle swizzle;

		if (swizzle.parse(swizzleText))
		{
			bool isValid = true;
			
			// determine size based on inputs
			
			int sx = 0;
			int sy = 0;
			int numChannels = 0;
			
			bool first = true;
			
			for (int i = 0; i < swizzle.numChannels; ++i)
			{
				auto & c = swizzle.channels[i];
				
				if (c.sourceIndex >= 0 && c.sourceIndex < 4 && channels[c.sourceIndex] != nullptr && c.elemIndex >= 0 && c.elemIndex < channels[c.sourceIndex]->numChannels)
				{
					const int csx = channels[c.sourceIndex]->sx;
					const int csy = channels[c.sourceIndex]->sy;
					
					//
					
					if (first)
					{
						first = false;
						
						sx = csx;
						sy = csy;
					}
					else if (mergeMode == kMergeMode_AppendChannels)
					{
						if (wrapMode == kWrapMode_Clamp)
						{
							sx = std::min(sx, csx);
							sy = std::min(sy, csy);
						}
						else if (wrapMode == kWrapMode_Cycle)
						{
							sx = std::max(sx, csx);
							sy = std::max(sy, csy);
						}
						else if (wrapMode == kWrapMode_PadZero)
						{
							sx = std::max(sx, csx);
							sy = std::max(sy, csy);
						}
					}
					else if (mergeMode == kMergeMode_ConcatenateValues)
					{
						sx += csx;
						
						 // fixme : require separate merge and wrap modes for each dimension .. ?
						
						if (wrapMode == kWrapMode_Clamp)
							sy = std::min(sy, csy);
						else
							sy = std::max(sy, csy);
					}
					
					//
					
					if (mergeMode == kMergeMode_AppendChannels)
					{
						numChannels++;
					}
					else
					{
						numChannels = 1;
					}
				}
			}
			
			const int size = sx * sy * numChannels;
			
			channelData.allocOnSizeChange(size);
			
			channelsOutput.size = size;
			channelsOutput.numChannels = 0;
			channelsOutput.sx = sx;
			channelsOutput.sy = sy;
			
			// merge
			
			first = false;
			
			float * __restrict channelDataPtr = channelData.data;
			
			if (mergeMode == kMergeMode_ConcatenateValues && numChannels > 0)
			{
				VfxChannel newChannel;
				newChannel.data = channelDataPtr;
				newChannel.continuous = false;
				
				channelsOutput.channels[channelsOutput.numChannels++] = newChannel;
			}
			
			for (int i = 0; i < swizzle.numChannels; ++i)
			{
				auto & c = swizzle.channels[i];

				if (c.sourceIndex >= 0 && c.sourceIndex < 4 && channels[c.sourceIndex] != nullptr && c.elemIndex >= 0 && c.elemIndex < channels[c.sourceIndex]->numChannels)
				{
					const int csx = channels[c.sourceIndex]->sx;
					const int csy = channels[c.sourceIndex]->sy;
					
					auto & channel = channels[c.sourceIndex]->channels[c.elemIndex];
					
					if (mergeMode == kMergeMode_AppendChannels && csx == sx && csy == sy)
					{
						channelsOutput.channels[channelsOutput.numChannels++] = channel;
						
						channelDataPtr += csx * csy;
					}
					else
					{
						if (mergeMode == kMergeMode_AppendChannels)
						{
							VfxChannel newChannel;
							newChannel.data = channelDataPtr;
							newChannel.continuous = channel.continuous;
							
							channelsOutput.channels[channelsOutput.numChannels++] = newChannel;
						}
						
						for (int ay = 0; ay < sy; ++ay)
						{
							int y = ay;
							
							if (wrapMode == kWrapMode_Cycle)
							{
								y %= channels[c.sourceIndex]->sy;
							}
							
							if (y < channels[c.sourceIndex]->sy)
							{
								if (mergeMode == kMergeMode_AppendChannels)
								{
									if (wrapMode == kWrapMode_Clamp)
									{
										Assert(sx <= csx);
										
										memcpy(channelDataPtr, channel.data, sx * sizeof(float));
										
										channelDataPtr += sx;
									}
									else if (wrapMode == kWrapMode_PadZero)
									{
										memcpy(channelDataPtr, channel.data, csx * sizeof(float));
										
										for (int x = csx; x < sx; ++x)
											channelDataPtr[x] = 0.f;
										
										channelDataPtr += sx;
									}
									else if (wrapMode == kWrapMode_Cycle)
									{
										for (int x = 0; x < sx; ++x)
										{
											channelDataPtr[x] = channel.data[x % csx];
										}
										
										channelDataPtr += sx;
									}
								}
								else if (mergeMode == kMergeMode_ConcatenateValues)
								{
									memcpy(channelDataPtr, channel.data, csx * sizeof(float));
								
									channelDataPtr += csx;
								}
							}
							else
							{
								Assert(wrapMode == kWrapMode_PadZero);
								
								if (mergeMode == kMergeMode_AppendChannels)
								{
									memset(channelDataPtr, 0, sx * sizeof(float));
									
									channelDataPtr += sx;
								}
								else if (mergeMode == kMergeMode_ConcatenateValues)
								{
									memset(channelDataPtr, 0, csx * sizeof(float));
									
									channelDataPtr += csx;
								}
							}
						}
					}
				}
				else
				{
					isValid = false;
				}
			}
			
			Assert(channelsOutput.numChannels == numChannels);
			Assert(channelDataPtr == channelData.data + channelData.size);
			
			if (isValid == false)
			{
				channelsOutput.reset();
			}
		}
	}
}
