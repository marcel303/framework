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

#if 0

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

VFX_NODE_TYPE(VfxNodeChannelMerge)
{
	typeName = "channel.merge";
	inEnum("mergeMode", "channelMergeMergeMode");
	inEnum("wrapMode", "channelMergeWrapMode");
	in("channel1", "channel");
	in("channel2", "channel");
	in("channel3", "channel");
	in("channel4", "channel");
	in("swizzle", "string");
	out("channel", "channel");
}

VfxNodeChannelMerge::VfxNodeChannelMerge()
	: VfxNodeBase()
	, channelData()
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_MergeMode, kVfxPlugType_Int);
	addInput(kInput_WrapMode, kVfxPlugType_Int);
	addInput(kInput_Channel1, kVfxPlugType_Channel);
	addInput(kInput_Channel2, kVfxPlugType_Channel);
	addInput(kInput_Channel3, kVfxPlugType_Channel);
	addInput(kInput_Channel4, kVfxPlugType_Channel);
	addInput(kInput_Swizzle, kVfxPlugType_String);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

void VfxNodeChannelMerge::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeChannelMerge);
	
	if (isPassthrough)
	{
		channelData.free();
		channelOutput.reset();
		return;
	}
	
	const MergeMode mergeMode = (MergeMode)getInputInt(kInput_MergeMode, 0);
	const WrapMode wrapMode = (WrapMode)getInputInt(kInput_WrapMode, 0);
	const VfxChannel * channel1 = getInputChannel(kInput_Channel1, nullptr);
	const VfxChannel * channel2 = getInputChannel(kInput_Channel2, nullptr);
	const VfxChannel * channel3 = getInputChannel(kInput_Channel3, nullptr);
	const VfxChannel * channel4 = getInputChannel(kInput_Channel4, nullptr);
	const char * swizzleText = getInputString(kInput_Swizzle, nullptr);

	const VfxChannel * channels[4] =
	{
		channel1,
		channel2,
		channel3,
		channel4
	};
	
	channelOutput.reset();
	
	VfxSwizzle swizzle;
	
	bool swizzleIsValid;
	
	if (swizzleText == nullptr)
	{
		swizzleIsValid = true;
		
		for (int i = 0; i < 4; ++i)
		{
			if (channels[i] != nullptr)
			{
				if (swizzle.numChannels < VfxSwizzle::kMaxChannels)
				{
					swizzle.channels[swizzle.numChannels].sourceIndex = i;
					
					swizzle.numChannels++;
				}
			}
		}
	}
	else
	{
		swizzleIsValid = swizzle.parse(swizzleText);
	}
	
	if (swizzleIsValid == false)
	{
		channelData.free();
		
		channelOutput.reset();
	}
	else
	{
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
			
			// determine allocation size
			
			int allocationSize = 0;
			
			for (int i = 0; i < swizzle.numChannels; ++i)
			{
				auto & c = swizzle.channels[i];

				if (c.sourceIndex >= 0 && c.sourceIndex < 4 && channels[c.sourceIndex] != nullptr && c.elemIndex >= 0 && c.elemIndex < channels[c.sourceIndex]->numChannels)
				{
					const int csx = channels[c.sourceIndex]->sx;
					const int csy = channels[c.sourceIndex]->sy;
					
					if (mergeMode == kMergeMode_AppendChannels && csx == sx && csy == sy)
					{
						// free !
					}
					else
					{
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
										
										allocationSize += sx;
									}
									else if (wrapMode == kWrapMode_PadZero)
									{
										allocationSize += sx;
									}
									else if (wrapMode == kWrapMode_Cycle)
									{
										allocationSize += sx;
									}
								}
								else if (mergeMode == kMergeMode_ConcatenateValues)
								{
									allocationSize += csx;
								}
							}
							else
							{
								Assert(wrapMode == kWrapMode_PadZero);
								
								if (mergeMode == kMergeMode_AppendChannels)
								{
									allocationSize += sx;
								}
								else if (mergeMode == kMergeMode_ConcatenateValues)
								{
									allocationSize += csx;
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
			
			//
			
			channelsOutput.size = sx * sy;
			channelsOutput.numChannels = 0;
			
			channelsOutput.sx = sx;
			channelsOutput.sy = sy;
			
			channelData.allocOnSizeChange(allocationSize);
			
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

void VfxNodeChannelMerge::getDescription(VfxNodeDescription & d)
{
	d.add("memory usage: %d values", channelData.size);
	d.add("output size: %dx%d x %d channels", channelsOutput.sx, channelsOutput.sy, channelsOutput.numChannels);
}

#endif
