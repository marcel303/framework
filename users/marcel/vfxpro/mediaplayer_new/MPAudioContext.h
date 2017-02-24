#pragma once

#include "MPAudioBuffer.h"
#include "MPPacketQueue.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

namespace MP
{
	class Context;

	class AudioContext
	{
	public:
		AudioContext();
		~AudioContext();

		bool Initialize(Context* context, size_t streamIndex);
		bool Destroy();

		size_t GetStreamIndex();
		double GetTime();

		bool FillAudioBuffer();
		bool RequestAudio(int16_t* out_samples, size_t frameCount, bool& out_gotAudio);

		bool IsQueueFull();
		bool AddPacket(AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool Depleted() const;

	//private: // FIXME.
		PacketQueue m_packetQueue;
		AudioBuffer m_audioBuffer;
		AVCodecContext* m_codecContext;
		AVCodec* m_codec;
		// TODO: Add AV codec conversion manager/state.

		size_t m_streamIndex;
		double m_time;
		size_t m_frameTime;

		bool m_initialized;
	};
};
