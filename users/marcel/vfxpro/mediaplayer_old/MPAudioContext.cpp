#include "MPAudioContext.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPUtil.h"

#define QUEUE_SIZE (4 * 3 * 10)

#define BUFFER_SIZE (AVCODEC_MAX_AUDIO_FRAME_SIZE / 2 * 2) // Maximum buffer size. If the buffer grows larger than this, audio artefacts will manifest.

// todo : move audio buffer to member. use aligned alloc
int16_t g_audioBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE / 2];

namespace MP
{
	AudioContext::AudioContext()
	{
		m_initialized = false;

		m_codecContext = 0;
		m_codec = 0;
	}

	AudioContext::~AudioContext()
	{
		Assert(m_initialized == false);
	}

	bool AudioContext::Initialize(Context* context, size_t streamIndex)
	{
		Assert(m_initialized == false);

		bool result = true;

		m_initialized = true;

		m_audioBuffer.SetBufferSize(BUFFER_SIZE);

		m_streamIndex = streamIndex;
		m_time = 0.0;
		m_frameTime = 0;
		m_availableFrameCount = 0;

		// Get codec context for audio stream.
		m_codecContext = context->GetFormatContext()->streams[m_streamIndex]->codec;

		Util::SetDefaultCodecContextOptions(m_codecContext);

		// Get codec for audio stream.
		m_codec = avcodec_find_decoder(m_codecContext->codec_id);
		if (!m_codec)
		{
			Assert(0);
			return false; // Codec not found
		}
		Debug::Print("Audio codec: %s.", m_codec->name);

		// Open codec.
		if (avcodec_open(m_codecContext, m_codec) < 0)
		{
			Assert(0);
			return false;
		}

		// Display codec info.
		Debug::Print("Audio: samplerate: %d.", m_codecContext->sample_rate);
		Debug::Print("Audio: channels: %d.",   m_codecContext->channels   );
		Debug::Print("Audio: framesize: %d.",  m_codecContext->frame_size ); // Number of samples/packet.

		// ------------

		// Setup resampling context.

		const int input_channels  = m_codecContext->channels;
		const int input_rate      = m_codecContext->sample_rate;
		const int output_channels = 2;
		const int output_rate     = 44100;

		// TODO: Maintain pointer to resampler, and apply resampling.
		ReSampleContext* resampler = audio_resample_init(output_channels, input_channels, output_rate, input_rate);
		//int audio_resample(ReSampleContext *s, short *output, short *input, int nb_samples);
		audio_resample_close(resampler);

		return result;
	}

	bool AudioContext::Destroy()
	{
		Assert(m_initialized == true);

		bool result = true;

		m_initialized = false;

		// Close audio codec context.
		if (m_codecContext)
			avcodec_close(m_codecContext);

		return result;
	}

	size_t AudioContext::GetStreamIndex()
	{
		return m_streamIndex;
	}

	double AudioContext::GetTime()
	{
		return m_time;
	}

	bool AudioContext::RequestAudio(int16_t* out_samples, size_t frameCount, bool& out_gotAudio)
	{
		Assert(out_samples);

		bool result = true;

		out_gotAudio = false;

		bool stop = false;

		Debug::Print("AUDIO: Begin request.");

		while (frameCount > 0 && stop == false)
		{
			Assert(m_audioBuffer.GetSampleCount() == m_availableFrameCount * m_codecContext->channels);

			if (m_audioBuffer.GetSampleCount() > 0)
			{
				size_t framesToRead = m_availableFrameCount;
				if (framesToRead > frameCount)
					framesToRead = frameCount;

				m_audioBuffer.ReadSamples(out_samples, framesToRead * m_codecContext->channels);
				out_samples += framesToRead * m_codecContext->channels;

				frameCount -= framesToRead;
				m_availableFrameCount -= framesToRead;
				m_frameTime += framesToRead;
				m_time = m_frameTime / (double)m_codecContext->sample_rate;

				Debug::Print("\tAUDIO: Read from buffer. Time = %03.3f", m_time);

				if (frameCount == 0)
				{
					Debug::Print("\tAUDIO: Read done.");
				}
			}
			else
			{
				if (m_packetQueue.GetSize() > 0)
				{
					Debug::Print("\tAUDIO: Decoding to provide more frames.");
					Debug::Print("\t\tAUDIO: Packet queue size: %d -> %d.", int(m_packetQueue.GetSize()), int(m_packetQueue.GetSize() - 1));

					AVPacket& packet = m_packetQueue.GetPacket();

					//m_time = packet.pts / 1000000.0;
					//m_time = av_q2d(m_codecContext->time_base) * packet.pts;

					if (ProcessPacket(packet) != true)
						result = false;

					m_packetQueue.PopFront();
				}
				else
				{
					for (size_t i = 0; i < frameCount * m_codecContext->channels; ++i)
						out_samples[i] = 0;
					Debug::Print("\tAUDIO: Silence?");
					stop = true;
				}
			}
		}

		Debug::Print("AUDIO: End request.");

		out_gotAudio = true;

		return result;
	}

	bool AudioContext::IsQueueFull()
	{
		return m_packetQueue.GetSize() > QUEUE_SIZE;
	}

	bool AudioContext::AddPacket(AVPacket& packet)
	{
		m_packetQueue.PushBack(packet);

		Debug::Print("PQAUDIO");

		return true;
	}

	bool AudioContext::ProcessPacket(AVPacket& packet)
	{
		Assert(packet.data);
		Assert(packet.size >= 0);

		int      bytesRemaining = 0;
		uint8_t* packetData     = 0;

		bytesRemaining = packet.size;
		packetData = packet.data;

		// Decode packet.
		while (bytesRemaining > 0)
		{
			// Decode data.
			int frameSize = 0;

			int bytesDecoded = avcodec_decode_audio(
				m_codecContext,
				g_audioBuffer,
				&frameSize,
				packetData,
				bytesRemaining);

			// Error?
			if (bytesDecoded < 0)
			{
				printf("Unable to decode audio packet.\n");
				bytesRemaining = 0;
				Assert(0);
			}
			else
			{
				bytesRemaining -= bytesDecoded;
				packetData += bytesDecoded;

				Assert(bytesDecoded >= 0);
				Assert(bytesRemaining >= 0);

				if (frameSize > 0)
				{
					if (m_codecContext->channels != 0)
					{
						size_t frameCount = frameSize / (m_codecContext->channels * sizeof(int16_t));

						m_audioBuffer.WriteSamples(g_audioBuffer, frameCount * m_codecContext->channels);

						m_availableFrameCount += frameCount;

						Assert(frameCount * (m_codecContext->channels * sizeof(int16_t)) == frameSize);

						Debug::Print("\t\tAUDIO: Decoded frame. Got %d frames.", int(frameCount));
					}
				}
				else
				{
					Assert(0);
				}
			}
		}

		// Free current packet.
		if (packet.data)
			av_free_packet(&packet);

		return true;
	}
};
