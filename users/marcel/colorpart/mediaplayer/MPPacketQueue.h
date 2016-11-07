#pragma once

#include <deque>
#include <ffmpeg/avformat.h>

namespace MP
{
	class PacketQueue
	{
	public:
		PacketQueue();
		~PacketQueue();

		void PushBack(AVPacket packet);
		void PopFront();

		size_t GetSize() const;
		bool IsEmpty() const;
		AVPacket& GetPacket();

		void Clear();

	private:
		std::deque<AVPacket> m_packets;
	};
};
