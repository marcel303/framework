/*
	Copyright (C) 2020 Marcel Smit
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

#include "MPAudioBuffer.h"
#include "MPAudioContext.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPPacketQueue.h"
#include "MPUtil.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswresample/swresample.h>
}

#define QUEUE_SIZE (4 * 3 * 10)

#if !defined(LIBAVCODEC_VERSION_MAJOR)
	#error LIBAVCODEC_VERSION_MAJOR not defined
#endif

namespace MP
{
	AudioContext::AudioContext()
		: m_packetQueue(nullptr)
		, m_audioBuffer(nullptr)
		, m_codecContext(nullptr)
		, m_codec(nullptr)
		, m_swrContext(nullptr)
		, m_outputChannelCount(0)
		, m_timeBase(0.0)
		, m_streamIndex(-1)
		, m_initialized(false)
	{
	}

	AudioContext::~AudioContext()
	{
		Assert(m_initialized == false);
		
		Assert(m_packetQueue == nullptr);
		Assert(m_audioBuffer == nullptr);
		Assert(m_codecContext == nullptr);
		Assert(m_codec == nullptr);
		Assert(m_swrContext == nullptr);
		Assert(m_outputChannelCount == 0);
	}

	bool AudioContext::Initialize(Context * context, const size_t streamIndex, const AudioOutputMode outputMode)
	{
		Assert(m_initialized == false);

		m_initialized = true;

		m_streamIndex = streamIndex;
		
		Assert(m_packetQueue == nullptr);
		m_packetQueue = new PacketQueue();
		
		Assert(m_audioBuffer == nullptr);
		m_audioBuffer = new AudioBuffer();
		
		AVCodecID codec_id = AV_CODEC_ID_NONE;
		
	#if LIBAVCODEC_VERSION_MAJOR >= 57
		AVCodecParameters * audioParams = context->GetFormatContext()->streams[m_streamIndex]->codecpar;
		if (!audioParams)
		{
			Debug::Print("Audio: failed to find audio params.");
			return false;
		}
		
		codec_id = audioParams->codec_id;
	#else
		codec_id = context->GetFormatContext()->streams[m_streamIndex]->codec->codec_id;
	#endif
	
		// Get codec for audio stream.
		m_codec = avcodec_find_decoder(codec_id);
		if (!m_codec)
		{
			Debug::Print("Audio: unable to find codec.");
			return false;
		}
		
		Debug::Print("Audio: codec: %s.", m_codec->name);
		
		// Get codec context for audio stream.
		m_codecContext = avcodec_alloc_context3(m_codec);
		if (!m_codecContext)
		{
			Debug::Print("Audio: failed to allocate codec context.");
			return false;
		}
		
	#if LIBAVCODEC_VERSION_MAJOR >= 57
		if (avcodec_parameters_to_context(m_codecContext, audioParams) < 0)
		{
			Debug::Print("Audio: failed to set params on codec context.");
			return false;
		}
	#endif

		// Open codec.
		if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0)
		{
			Debug::Print("Audio: failed to open codec context.");
			return false;
		}

		// Display codec info.
		Debug::Print("Audio: samplerate: %d.", m_codecContext->sample_rate);
		Debug::Print("Audio: channels: %d.", m_codecContext->channels);
		Debug::Print("Audio: framesize: %d.", m_codecContext->frame_size); // Number of samples/packet.
		
	#if LIBAVCODEC_VERSION_MAJOR >= 57
		const int64_t outputChannelLayout =
			outputMode == kAudioOutputMode_Mono ? AV_CH_LAYOUT_MONO :
			outputMode == kAudioOutputMode_Stereo ? AV_CH_LAYOUT_STEREO :
			audioParams->channel_layout;
	#else
		const int64_t outputChannelLayout =
			outputMode == kAudioOutputMode_Mono ? AV_CH_LAYOUT_MONO :
			outputMode == kAudioOutputMode_Stereo ? AV_CH_LAYOUT_STEREO :
			m_codecContext->channel_layout;
	#endif

	#if LIBAVCODEC_VERSION_MAJOR >= 57
		Assert(m_swrContext == nullptr);
		int inputChannelLayout = audioParams->channel_layout;
		if (inputChannelLayout == 0)
			inputChannelLayout = (1 << audioParams->channels) - 1;
		m_swrContext = swr_alloc_set_opts(nullptr,
			// output
			outputChannelLayout,
			AV_SAMPLE_FMT_S16,
			m_codecContext->sample_rate,
			// input
			inputChannelLayout,
			(AVSampleFormat)audioParams->format,
			audioParams->sample_rate,
			// log
			0, nullptr);
	#else
		Assert(m_swrContext == nullptr);
		int inputChannelLayout = m_codecContext->channel_layout;
		if (inputChannelLayout == 0)
			inputChannelLayout = (1 << m_codecContext->channels) - 1;
		m_swrContext = swr_alloc_set_opts(nullptr,
			// output
			outputChannelLayout,
			AV_SAMPLE_FMT_S16,
			m_codecContext->sample_rate,
			// input
			inputChannelLayout,
			m_codecContext->sample_fmt,
			m_codecContext->sample_rate,
			// log
			0, nullptr);
	#endif
		
		if (!m_swrContext)
		{
			Debug::Print("Audio: failed to alloc/init swr context.");
			return false;
		}
		
		if (swr_init(m_swrContext) < 0)
		{
			Debug::Print("Audio: failed to init swr context.");
			return false;
		}
		
		m_outputChannelCount =
			outputChannelLayout == AV_CH_LAYOUT_MONO ? 1 :
			outputChannelLayout == AV_CH_LAYOUT_STEREO ? 2 :
			m_codecContext->channels;
		
		Debug::Print("Audio: output channels: %d.", m_outputChannelCount);
		
		m_timeBase = av_q2d(context->GetFormatContext()->streams[streamIndex]->time_base);
		
		return true;
	}

	bool AudioContext::Destroy()
	{
		Assert(m_initialized == true);

		bool result = true;

		m_initialized = false;

		m_outputChannelCount = 0;
		
		if (m_swrContext)
		{
			swr_free(&m_swrContext);
			m_swrContext = nullptr;
		}
		
		// Close audio codec context.
		if (m_codecContext)
		{
			avcodec_free_context(&m_codecContext);
			m_codecContext = nullptr;
		}
		
		if (m_codec)
		{
			m_codec = nullptr;
		}
		
		if (m_audioBuffer)
		{
			delete m_audioBuffer;
			m_audioBuffer = nullptr;
		}
		
		if (m_packetQueue)
		{
			delete m_packetQueue;
			m_packetQueue = nullptr;
		}

		return result;
	}

	size_t AudioContext::GetStreamIndex() const
	{
		return m_streamIndex;
	}

	bool AudioContext::FillAudioBuffer()
	{
		bool result = true;

		while (m_packetQueue->GetSize() > 0)
		{
			Debug::Print("\tAudio: decoding to provide more frames.");
			Debug::Print("\t\tAudio: packet queue size: %d -> %d.", int(m_packetQueue->GetSize()), int(m_packetQueue->GetSize() - 1));

			AVPacket & packet = m_packetQueue->GetPacket();

			if (ProcessPacket(packet) != true)
				result = false;

			m_packetQueue->PopFront();
		}

		return result;
	}

	bool AudioContext::RequestAudio(int16_t * out_samples, const size_t frameCount, bool & out_gotAudio, double & out_audioTime)
	{
		size_t numFramesLeft = frameCount;
		
		Assert(out_samples);

		bool result = true;

		out_gotAudio = false;

		bool stop = false;

		Debug::Print("Audio: begin request.");

		while (stop == false)
		{
			const size_t numSamplesToRead = numFramesLeft * m_outputChannelCount;
			
			// read samples from the audio buffer
			
			size_t numSamplesRead;
			const bool hasMore = m_audioBuffer->ReadSamples(out_samples, numSamplesToRead, numSamplesRead, out_audioTime);
			
			// update the output buffer pointer and the number of frames left
			
			const size_t numFramesRead = numSamplesRead / m_outputChannelCount;
			Assert((numSamplesRead % m_outputChannelCount) == 0);
			Assert(numFramesRead * m_outputChannelCount == numSamplesRead);

			if (numFramesRead > 0)
			{
				Debug::Print("\tAudio: read from buffer. time: %03.3f", out_audioTime);
				
				out_samples += numFramesRead * m_outputChannelCount;
				
				numFramesLeft -= numFramesRead;
			}
			
			if (hasMore)
			{
				// there's more audio data left inside the audio buffer. loop until done
				
				if (numFramesLeft == 0)
				{
					Debug::Print("\tAudio: read done.");
					
					stop = true;
				}
			}
			else
			{
				// the audio buffer is empty. calculate the number of remaining samples, set the remaining
				// samples to zero, and end the loop
				
				const size_t numSamplesRemaining = numFramesLeft * m_outputChannelCount;
				
				if (numSamplesRemaining > 0)
				{
					memset(out_samples, 0, numSamplesRemaining * sizeof(int16_t));
				}
				
				stop = true;
			}
		}

		Debug::Print("Audio: end request.");

		out_gotAudio = true;

		return result;
	}

	bool AudioContext::IsQueueFull() const
	{
		return m_packetQueue->GetSize() >= QUEUE_SIZE;
	}

	bool AudioContext::AddPacket(AVPacket & packet)
	{
		m_packetQueue->PushBack(packet);

		Debug::Print("PQAUDIO");

		return true;
	}

	bool AudioContext::ProcessPacket(AVPacket & _packet)
	{
		Assert(_packet.data);
		Assert(_packet.size >= 0);

		AVPacket packet = _packet;
		AVFrame * frame = av_frame_alloc();
		
		int bytesRemaining = packet.size;
		
		// Decode packet.
		while (bytesRemaining > 0)
		{
			// Decode data.
			
			int gotFrame = 0;
			int bytesDecoded = avcodec_decode_audio4(
				m_codecContext,
				frame,
				&gotFrame,
				&packet);

			// Error?
			if (bytesDecoded < 0)
			{
				Debug::Print("Audio: unable to decode audio packet.");
				bytesRemaining = 0;
				Assert(0);
			}
			else
			{
				packet.data += bytesDecoded;
				packet.size -= bytesDecoded;
				
				bytesRemaining -= bytesDecoded;

				Assert(bytesDecoded >= 0);
				Assert(bytesRemaining >= 0);
				
				if (gotFrame && frame->nb_samples > 0)
				{
				#if 1
					if (m_outputChannelCount != 0)
					{
						AudioBufferSegment segment;
						
						int16_t * __restrict dst = segment.m_samples;
						
						swr_convert(m_swrContext, (uint8_t**)&dst, frame->nb_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
						
						segment.m_numSamples = frame->nb_samples * m_outputChannelCount;
						
						segment.m_time = av_frame_get_best_effort_timestamp(frame) * m_timeBase;

						m_audioBuffer->AddSegment(segment);

						Debug::Print("\t\tAudio: decoded frame. got %d frames.", int(frame->nb_samples));
					}
				#else
					if (m_codecContext->channels != 0)
					{
						AudioBufferSegment segment;
						
						int16_t * __restrict dst = segment.m_samples;
						
						for (int i = 0; i < frame->nb_samples; ++i)
						{
							for (int c = 0; c < m_codecContext->channels; ++c)
							{
								if (frame->format == AV_SAMPLE_FMT_S16P)
								{
									const int16_t * __restrict src = (int16_t*)frame->data[c];
									
									*dst++ = src[i];
								}
								else if (frame->format == AV_SAMPLE_FMT_S32P)
								{
									const int32_t * __restrict src = (int32_t*)frame->data[c];
									
									*dst++ = src[i] >> 16;
								}
								else if (frame->format == AV_SAMPLE_FMT_FLTP)
								{
									const float * __restrict src = (float*)frame->data[c];
									
									//Assert(src[i] >= -1.f && src[i] <= +1.f);
									
									*dst++ = src[i] * ((1 << 15) - 1);
								}
								else
								{
									Assert(false);
									
									*dst++ = 0;
								}
							}
						}
						
						segment.m_numSamples = frame->nb_samples * m_codecContext->channels;
						
						segment.m_time = av_frame_get_best_effort_timestamp(frame) * m_timeBase;

						m_audioBuffer->AddSegment(segment);

						Debug::Print("\t\tAudio: decoded frame. got %d frames.", int(frame->nb_samples));
					}
				#endif
				}
				else
				{
					Assert(gotFrame == false);
				}
			}
		}
		
		av_frame_unref(frame);
		frame = nullptr;
		
		return true;
	}

	bool AudioContext::Depleted() const
	{
		return m_audioBuffer->Depleted() && (m_packetQueue->GetSize() == 0);
	}
};
