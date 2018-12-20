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

#include "artnet.h"
#include "oscEndpointMgr.h"
#include "vfxNodeArtnet.h"

// todo : add DMX setup resource

VFX_NODE_TYPE(VfxNodeArtnet)
{
	typeName = "artnet";
	
	in("endpoint", "string");
	in("universe", "int");
	in("sequenced", "bool");
	in("channel", "channel");
	in("maxRate", "int", "30");
}

VfxNodeArtnet::VfxNodeArtnet()
	: VfxNodeBase()
	, timeSinceLastUpdate(0.f)
	, sequenceNumber(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_EndpointName, kVfxPlugType_String);
	addInput(kInput_Universe, kVfxPlugType_Int);
	addInput(kInput_UseSequenceNumbers, kVfxPlugType_Bool);
	addInput(kInput_Channel, kVfxPlugType_Channel);
	addInput(kInput_MaxUpdateRate, kVfxPlugType_Int);
}

VfxNodeArtnet::~VfxNodeArtnet()
{
}

void VfxNodeArtnet::tick(const float dt)
{
	const char * endpointName = getInputString(kInput_EndpointName, "");
	const int universe = getInputInt(kInput_Universe, 0);
	const bool sequenced = getInputBool(kInput_UseSequenceNumbers, false);
	const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
	const int maxUpdateRate = getInputInt(kInput_MaxUpdateRate, 30);
	
	// fetch endpoint

	OscSender * oscSender = g_oscEndpointMgr.findSender(endpointName);
	
	if (oscSender == nullptr || channel == nullptr || maxUpdateRate <= 0)
	{
		timeSinceLastUpdate = 0.f;
		
		return;
	}

	// update timer

	timeSinceLastUpdate += dt;

	// check if enough time has elapsed since last update, compared to the max update rate

	if (timeSinceLastUpdate >= 1.f / maxUpdateRate)
	{
		timeSinceLastUpdate = 0.f;

		// convert channel data to DMX values

		uint8_t values[512];

		const int numValues = clamp(channel->size, 0, 512);

		for (int i = 0; i < numValues; ++i)
			values[i] = clamp((int)roundf(channel->data[i] * 255.f), 0, 255);
		
		// increment sequence number, or set it to zero when sequencing is disabled
		
		if (sequenced)
		{
			sequenceNumber++;
			
			if (sequenceNumber == 0)
				sequenceNumber++;
		}
		else
		{
			sequenceNumber = 0;
		}
		
		// construct ArtNet packet from channel
		
		ArtnetPacket packet;
		
		const uint8_t physical = 0;
		
		if (packet.makeDMX512(
			values, numValues,
			sequenceNumber, physical, universe))
		{
			// send packet
			
			oscSender->send(packet.data, packet.dataSize);
		}
	}
}
