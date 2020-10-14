#include "audioStreamToTcp.h"

#include "nodeDiscovery.h"

#include "Debugging.h"
#include "Log.h"

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <netinet/ip.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
#endif

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
	const SampleFormat in_sampleFormat,
	const ProvideFunction & in_provideFunction)
{
	provideFunction = in_provideFunction;
	numBuffers = in_numBuffers;
	numFramesPerBuffer = in_numFramesPerBuffer;
	numChannelsPerFrame = in_numChannelsPerFrame;
	sampleFormat = in_sampleFormat;
	
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
			// set TCP_SENDMOREACKS for audio streamers, as this requires the other side to hold less data for (potential) retransmission
			const int sock_value = 1;
			setsockopt(tcpConnection.sock, IPPROTO_TCP, TCP_SENDMOREACKS, (const char*)&sock_value, sizeof(sock_value));
		}

		LOG_DBG("frame size: %d", numFramesPerBuffer * numChannelsPerFrame * sizeof(int16_t));
		
		const int numSamplesPerBuffer = numFramesPerBuffer * numChannelsPerFrame;
		const int sampleSize =
			sampleFormat == kSampleFormat_S16 ? 2 :
			sampleFormat == kSampleFormat_Float ? 4 :
			-1;
		const int bufferSize = numSamplesPerBuffer * sampleSize;
		void * __restrict samples = alloca(bufferSize);
		
		while (tcpConnection.wantsToStop.load() == false)
		{
			// generate some audio data
			
			provideFunction(samples, numFramesPerBuffer, numChannelsPerFrame);
			
			int16_t * __restrict data = nullptr;
			
			if (sampleFormat == kSampleFormat_S16)
			{
				data = (int16_t*)samples;
			}
			else if (sampleFormat == kSampleFormat_Float)
			{
				data = (int16_t*)alloca(numSamplesPerBuffer * sizeof(int16_t));
				
				const float * __restrict src = (float*)samples;
				int16_t * __restrict dst = data;
				
				for (int i = 0; i < numSamplesPerBuffer; ++i)
				{
					float value = *src++;
					if (value < 0.f)
						value = 0.f;
					else if (value > 1.f)
						value = 1.f;
					
					*dst++ = int16_t(value * float((1 << 15) - 1));
				}
			}
			else
			{
				Assert(false);
			}
			
			// perform volume mixing
			
			const int volume14 = volume.load() * (1 << 14);
			
			for (int i = 0; i < numSamplesPerBuffer; ++i)
				data[i] = (data[i] * volume14) >> 14;
			
			// send data
			
			if (send(tcpConnection.sock, (const char*)data, bufferSize, 0) < bufferSize)
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
