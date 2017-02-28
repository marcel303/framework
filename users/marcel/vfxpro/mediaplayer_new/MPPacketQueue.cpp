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
