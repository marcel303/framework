#pragma once

struct AVCodec;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;
struct SwsContext;

namespace MP
{
	class AudioBuffer;
	class AudioContext;
	class Context;
	class PacketQueue;
	class VideoBuffer;
	class VideoContext;
	class VideoFrame;
	
	enum OutputMode
	{
		kOutputMode_RGBA,
		kOutputMode_PlanarYUV
	};
}
