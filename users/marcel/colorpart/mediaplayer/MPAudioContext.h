#pragma once

#include "MPAudioBuffer.h"
#include "MPPacketQueue.h"
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

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

		bool RequestAudio(int16_t* out_samples, size_t frameCount, bool& out_gotAudio);

		bool IsQueueFull();
		bool AddPacket(AVPacket& packet);
		bool ProcessPacket(AVPacket& packet);
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
		size_t m_availableFrameCount;

		bool m_initialized;
	};
};
