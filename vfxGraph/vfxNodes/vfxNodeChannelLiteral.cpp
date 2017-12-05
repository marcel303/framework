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

#include "Parse.h"
#include "vfxNodeBase.h"

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

struct VfxNodeChannelLiteral : VfxNodeBase
{
	enum Input
	{
		kInput_Text,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Channels,
		kOutput_COUNT
	};
	
	VfxChannelData channelData;
	
	VfxChannels channelOutput;
	
	VfxNodeChannelLiteral()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Text, kVfxPlugType_String);
		addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const char * text = getInputString(kInput_Text, nullptr);
		
		if (isPassthrough || text == nullptr)
		{
			channelData.free();
			
			channelOutput.reset();
		}
		else
		{
			std::vector<std::string> parts;
			splitString(text, parts, ' ');
			
			channelData.allocOnSizeChange(parts.size());
			
			for (size_t i = 0; i < parts.size(); ++i)
			{
				const float value = Parse::Float(parts[i]);
				
				channelData.data[i] = value;
			}
			
			channelOutput.setDataContiguous(channelData.data, false, channelData.size, 1);
		}
	}
};

VFX_NODE_TYPE(VfxNodeChannelLiteral)
{
	typeName = "channel.literal";
	
	in("text", "string");
	out("channels", "channels");
}
