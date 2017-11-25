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

#include "Debugging.h"
#include "MPDebug.h"
#include "MPPacketQueue.h"

// TODO: Make packet queue maintain total size of buffered data (packet.size) &
//       query it instead of # packets in determining whether the packet queue is full yes/no.

namespace MP
{
	PacketQueue::PacketQueue()
	{
	}

	PacketQueue::~PacketQueue()
	{
		while (GetSize() > 0)
			PopFront();
	}

	void PacketQueue::PushBack(const AVPacket & packet)
	{
		AVPacket ref;
		av_init_packet(&ref);
		
		if (av_packet_ref(&ref, &packet) < 0)
			Debug::Print("av_packet_ref failed");
		else
			m_packets.push_back(ref);
	}

	void PacketQueue::PopFront()
	{
		Assert(m_packets.size() > 0);

		av_packet_unref(&m_packets.front());

		m_packets.pop_front();
	}

	size_t PacketQueue::GetSize() const
	{
		return m_packets.size();
	}

	bool PacketQueue::IsEmpty() const
	{
		return m_packets.empty();
	}

	AVPacket & PacketQueue::GetPacket()
	{
		Assert(m_packets.size() > 0);

		return m_packets.front();
	}

	void PacketQueue::Clear()
	{
		while (m_packets.size() > 0)
			PopFront();
	}
};
