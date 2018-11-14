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

#pragma once

#include <stdint.h>

/*
ArtNet is a protocol used to send light and media data over udp/ip.

The implementation provided here is very minimal. You will need to manage your udp/ip socket(s) yourself.
ArtnetPacket merely assists with constructing ArtNet data packets, ready to be sent over the network.

ArtNet commonly uses port 6454.

Also, this implementation only focuses on DMX right now.
*/

struct ArtnetPacket
{
	static const int kMaxSize = 1024;

	uint8_t data[kMaxSize]; ///< The data to be sent to the ArtNet client.
	uint16_t dataSize = 0;  ///< The size of the packet. Use this size when sending the data over UDP.

	/*
	Create a ArtNet-DMX packet with 1 to 512 DMX values.
	@values: The array of DMX values between 0 and 255.
	@numValues: The number of values in the DMX values array. This can be anywhere between 1 and 512
	@sequenceNumber: The DMX packet sequence number. If 0, the sequence number is ignored. If between 1 and 255, the receiver will use these numbers to re-order data packets.
	@physical: The physical ID of the sender. Usually ignored and 0.
	@universe: The universe of the DMX values. Usually 0 when the DMX setup isn't segmented into universes.
	*/
	bool makeDMX512(
		const uint8_t * __restrict values, const int numValues,
		const uint8_t sequenceNumber = 0, const uint8_t physical = 0, const uint16_t universe = 0);
};
