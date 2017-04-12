#pragma once

#include "MPForward.h"
#include <stdlib.h>

struct SwrContext;

namespace MP
{
	class Context;

	class AudioContext
	{
	public:
		AudioContext();
		~AudioContext();

		bool Initialize(Context * context, const size_t streamIndex);
		bool Destroy();

		size_t GetStreamIndex() const;
		double GetTime() const;

		bool FillAudioBuffer();
		bool RequestAudio(int16_t * out_samples, const size_t frameCount, bool & out_gotAudio);

		bool IsQueueFull() const;
		bool AddPacket(AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool Depleted() const;

	//private: // FIXME.
		PacketQueue * m_packetQueue;
		AudioBuffer * m_audioBuffer;
		AVCodecContext * m_codecContext;
		AVCodec * m_codec;
		SwrContext * m_swrContext;

		size_t m_streamIndex;
		double m_time;
		size_t m_frameTime;

		bool m_initialized;
	};
};
