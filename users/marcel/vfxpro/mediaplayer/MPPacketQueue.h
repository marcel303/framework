#pragma once

#include <deque>

extern "C"
{
	#include <libavformat/avformat.h>
}

namespace MP
{
	class PacketQueue
	{
	public:
		PacketQueue();
		~PacketQueue();

		void PushBack(AVPacket packet);
		void PopFront();

		size_t GetSize();
		AVPacket& GetPacket();

		void Clear();

	private:
		std::deque<AVPacket> m_packets;
	};
};
