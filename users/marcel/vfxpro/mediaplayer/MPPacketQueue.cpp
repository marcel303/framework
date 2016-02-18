#include "Debugging.h"
#include "MPPacketQueue.h"

// TODO: Make packet queue maintain total size of buffered data (packet.size) &
//       query it instead of # packets in determining whther the packet queue is full yes/no.

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

	void PacketQueue::PushBack(AVPacket packet)
	{
		av_dup_packet(&packet);

		m_packets.push_back(packet);
	}

	void PacketQueue::PopFront()
	{
		Assert(m_packets.size() > 0);

		av_free_packet(&m_packets.front());

		m_packets.pop_front();
	}

	size_t PacketQueue::GetSize()
	{
		return m_packets.size();
	}

	AVPacket& PacketQueue::GetPacket()
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
