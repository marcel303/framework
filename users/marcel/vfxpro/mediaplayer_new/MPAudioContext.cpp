#include "MPAudioContext.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPUtil.h"

#define QUEUE_SIZE (4 * 3 * 10)

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

		m_streamIndex = streamIndex;
		m_time = 0.0;
		m_frameTime = 0;
		
		AVCodecParameters * audioParams = context->GetFormatContext()->streams[m_streamIndex]->codecpar;
		m_codec = avcodec_find_decoder(audioParams->codec_id);
		
		// Get codec for audio stream.
		if (!m_codec)
		{
			Assert(0);
			return false; // Codec not found
		}
		
		Debug::Print("Audio codec: %s.", m_codec->name);
		
		// Get codec context for audio stream.
		m_codecContext = avcodec_alloc_context3(m_codec);
		avcodec_parameters_to_context(m_codecContext, audioParams);

		//Util::SetDefaultCodecContextOptions(m_codecContext);

		// Open codec.
		if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0)
		{
			Assert(0);
			return false;
		}

		// Display codec info.
		Debug::Print("Audio: samplerate: %d.", m_codecContext->sample_rate);
		Debug::Print("Audio: channels: %d.",   m_codecContext->channels   );
		Debug::Print("Audio: framesize: %d.",  m_codecContext->frame_size ); // Number of samples/packet.
		
		return result;
	}

	bool AudioContext::Destroy()
	{
		Assert(m_initialized == true);

		bool result = true;

		m_initialized = false;

		// Close audio codec context.
		if (m_codecContext)
		{
			avcodec_close(m_codecContext);
			
			avcodec_free_context(&m_codecContext);
			m_codecContext = nullptr;
		}
		
		m_codec = nullptr;

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

	bool AudioContext::FillAudioBuffer()
	{
		bool result = true;

		while (m_packetQueue.GetSize() > 0)
		{
			Debug::Print("\tAUDIO: Decoding to provide more frames.");
			Debug::Print("\t\tAUDIO: Packet queue size: %d -> %d.", int(m_packetQueue.GetSize()), int(m_packetQueue.GetSize() - 1));

			AVPacket & packet = m_packetQueue.GetPacket();

			if (ProcessPacket(packet) != true)
				result = false;

			m_packetQueue.PopFront();
		}

		return result;
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
			size_t numSamples = frameCount * m_codecContext->channels;

			if (!m_audioBuffer.ReadSamples(out_samples, numSamples))
			{
				stop = true;
			}
			else
			{
				const size_t numFrames = numSamples / m_codecContext->channels;

				out_samples += numFrames * m_codecContext->channels;

				frameCount -= numFrames;
				m_frameTime += numFrames;
				m_time = m_frameTime / (double)m_codecContext->sample_rate;

				Debug::Print("\tAUDIO: Read from buffer. Time = %03.3f", m_time);

				if (frameCount == 0)
				{
					Debug::Print("\tAUDIO: Read done.");
				}
			}
		}

		Debug::Print("AUDIO: End request.");

		out_gotAudio = true;

		return result;
	}

	bool AudioContext::IsQueueFull()
	{
		return m_packetQueue.GetSize() >= QUEUE_SIZE;
	}

	bool AudioContext::AddPacket(AVPacket & packet)
	{
		m_packetQueue.PushBack(packet);

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
			int frameSize = 0;
			
			// todo : decode into segment
			
			AudioBufferSegment segment;
			
			//AVFrame frame;
			//frame.data[0] = segment.data;
			//frame.linesize[0] = segment.size;
			
			int gotFrame = 0;
			int bytesDecoded = avcodec_decode_audio4(
				m_codecContext,
				frame,
				&gotFrame,
				&packet);

			// Error?
			if (bytesDecoded < 0)
			{
				Debug::Print("Unable to decode audio packet.\n");
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

				if (frameSize > 0)
				{
					if (m_codecContext->channels != 0)
					{
						size_t frameCount = frameSize / (m_codecContext->channels * sizeof(int16_t));

						segment.m_numSamples = frameCount * m_codecContext->channels;

						m_audioBuffer.AddSegment(segment);

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
		
		av_frame_unref(frame);
		frame = nullptr;
		
		return true;
	}

	bool AudioContext::Depleted() const
	{
		return m_audioBuffer.Depleted() && (m_packetQueue.GetSize() == 0);
	}
};
