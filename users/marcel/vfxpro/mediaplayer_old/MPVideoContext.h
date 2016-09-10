#pragma once

#include "MPPacketQueue.h"
#include "MPVideoBuffer.h"
#include "Types.h"
#include <ffmpeg_old/avcodec.h>
#include <ffmpeg_old/avformat.h>

namespace MP
{
	class Context;

	class VideoContext
	{
	public:
		VideoContext();

		bool Initialize(Context * context, const size_t streamIndex, const bool outputYuv);
		bool Destroy();

		size_t GetStreamIndex() const;
		double GetTime() const;

		void FillVideoBuffer();

		bool RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo);

		bool IsQueueFull() const;
		bool AddPacket(const AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool AdvanceToTime(const double time, VideoFrame ** out_currentFrame);
		bool Depleted() const;

	//private:
		bool Convert(VideoFrame * out_frame);

		PacketQueue m_packetQueue;
		AVCodecContext * m_codecContext;
		AVCodec * m_codec;
		AVFrame * m_tempFrame; ///< Temporary frame for decoder. The results stored in this frame are converted to RGB and stored in the final frame.
		uint8_t * m_tempFrameBuffer; ///< Frame buffer for temp frame.
		VideoBuffer m_videoBuffer; // TODO: Init/destroy.

		size_t m_streamIndex;
		bool m_outputYuv;
		double m_time;
		size_t m_frameCount;

		bool m_initialized;
	};
};
