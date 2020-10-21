#include "audioStreamToTcp.h"

#include "Debugging.h"
#include "Log.h"

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <netinet/ip.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
#endif

static void floatToInt8(const float * __restrict src, int8_t * __restrict dst, const int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		float value = src[i];
		
		if (value < -1.f)
			value = -1.f;
		else if (value > 1.f)
			value = 1.f;

		const int value_int = int(value * float((1 << 7) - 1));
		//Assert(value_int >= INT8_MIN && value_int <= INT8_MAX);
		
		dst[i] = int8_t(value_int);
	}
}

static void floatToInt16(const float * __restrict src, int16_t * __restrict dst, const int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		float value = src[i];
		
		if (value < -1.f)
			value = -1.f;
		else if (value > 1.f)
			value = 1.f;

		const int value_int = int(value * float((1 << 15) - 1));
		//Assert(value_int >= INT16_MIN && value_int <= INT16_MAX);
		
		dst[i] = int16_t(value_int);
	}
}

static void int16ToInt8(const int16_t * __restrict src, int8_t * __restrict dst, const int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		dst[i] = src[i] >> 8;
	}
}

AudioStreamToTcp::AudioStreamToTcp()
	: volume(1.f)
{
}

bool AudioStreamToTcp::init(
	const uint32_t ipAddress,
	const uint16_t tcpPort,
	const int in_numBuffers,
	const int in_numFramesPerBuffer,
	const int in_numChannelsPerFrame,
	const SampleFormat in_provideSampleFormat,
	const SampleFormat in_networkSampleFormat,
	const ProvideFunction & in_provideFunction)
{
	provideFunction = in_provideFunction;
	numBuffers = in_numBuffers;
	numFramesPerBuffer = in_numFramesPerBuffer;
	numChannelsPerFrame = in_numChannelsPerFrame;
	provideSampleFormat = in_provideSampleFormat;
	networkSampleFormat = in_networkSampleFormat;
	
	ThreadedTcpConnection::Options options;
	options.noDelay = true;
	
	return tcpConnection.init(ipAddress, tcpPort, options, [this]()
	{
		{
			// tell the TCP stack to use a specific buffer size. usually the TCP stack is
			// configured to use a rather large buffer size to increase bandwidth. we want
			// to keep latency down however, so we reduce the buffer size here
			
			const int sock_value =
				numBuffers  *         /* N times buffered */
				numFramesPerBuffer  * /* frame count */
				numChannelsPerFrame * /* stereo */
				sizeof(int16_t)       /* sample size */;
			setsockopt(tcpConnection.sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sock_value, sizeof(sock_value));
		}
		
		{
			// set TCP_SENDMOREACKS/TCP_QUICKACK for audio streamers, as this requires the other side to hold less data for (potential) retransmission
		#if defined(MACOS)
			const int sock_opt = TCP_SENDMOREACKS;
		#else
			const int sock_opt = TCP_QUICKACK;
		#endif
			const int sock_value = 1;
			setsockopt(tcpConnection.sock, IPPROTO_TCP, sock_opt, (const char*)&sock_value, sizeof(sock_value));
		}

		LOG_DBG("frame size: %d", numFramesPerBuffer * numChannelsPerFrame * sizeof(int16_t));
		
		const int numSamplesPerBuffer = numFramesPerBuffer * numChannelsPerFrame;
		const int provideSampleSize =
			provideSampleFormat == kSampleFormat_S8 ? 1 :
			provideSampleFormat == kSampleFormat_S16 ? 2 :
			provideSampleFormat == kSampleFormat_Float ? 4 :
			-1;
		const int networkSampleSize =
			networkSampleFormat == kSampleFormat_S8 ? 1 :
			networkSampleFormat == kSampleFormat_S16 ? 2 :
			-1;
		const int provideBufferSize = numSamplesPerBuffer * provideSampleSize;
		const int networkBufferSize = numSamplesPerBuffer * networkSampleSize;
		
		void * __restrict provideBuffer = alloca(provideBufferSize);
		void * __restrict networkBuffer = alloca(networkBufferSize);
		
		while (tcpConnection.wantsToStop.load() == false)
		{
			// generate some audio data
			
			provideFunction(provideBuffer, numFramesPerBuffer, numChannelsPerFrame);
			
			void * __restrict data = nullptr;
			
			if (networkSampleFormat == provideSampleFormat)
			{
				data = provideBuffer;
			}
			else
			{
				data = networkBuffer;
				
				if (provideSampleFormat == kSampleFormat_Float)
				{
					if (networkSampleFormat == kSampleFormat_S8)
					{
						floatToInt8((float*)provideBuffer, (int8_t*)networkBuffer, numSamplesPerBuffer);
					}
					else if (networkSampleFormat == kSampleFormat_S16)
					{
						floatToInt16((float*)provideBuffer, (int16_t*)networkBuffer, numSamplesPerBuffer);
					}
					else
					{
						Assert(false);
					}
				}
				else if (provideSampleFormat == kSampleFormat_S16)
				{
					if (networkSampleFormat == kSampleFormat_S8)
					{
						int16ToInt8((int16_t*)provideBuffer, (int8_t*)networkBuffer, numSamplesPerBuffer);
					}
					else
					{
						Assert(false);
					}
				}
				else
				{
					Assert(false);
				}
			}
			
			// perform volume mixing
			
			if (networkSampleFormat == kSampleFormat_S8)
			{
				const int volume14 = int(volume.load() * (1 << 14));
				int8_t * data_int = (int8_t*)data;
				for (int i = 0; i < numSamplesPerBuffer; ++i)
					data_int[i] = (data_int[i] * volume14) >> 14;
			}
			else if (networkSampleFormat == kSampleFormat_S16)
			{
				const int volume14 = int(volume.load() * (1 << 14));
				int16_t * data_int = (int16_t*)data;
				for (int i = 0; i < numSamplesPerBuffer; ++i)
					data_int[i] = (data_int[i] * volume14) >> 14;
			}
			else if (networkSampleFormat == kSampleFormat_S8)
			{
				const float volume_value = volume.load();
				float * data_float = (float*)data;
				for (int i = 0; i < numSamplesPerBuffer; ++i)
					data_float[i] = data_float[i] * volume_value;
			}
			
			// send data
			
			if (send(tcpConnection.sock, (const char*)data, networkBufferSize, 0) < networkBufferSize)
			{
				LOG_ERR("failed to send data");
				tcpConnection.wantsToStop = true;
			}
		}
	});
	
	return true;
}

void AudioStreamToTcp::beginShutdown()
{
	tcpConnection.beginShutdown();
}

void AudioStreamToTcp::waitForShutdown()
{
	tcpConnection.waitForShutdown();
}
