#include "audioStreamToTcp.h"

#include "nodeDiscovery.h"

#include "audiostream/AudioStreamVorbis.h"
#include "Log.h"

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <sys/socket.h>
#endif

bool Test_TcpToI2S::init(const uint32_t ipAddress, const uint16_t tcpPort, const char * filename)
{
	audioStream.Open(filename, true);
	
	ThreadedTcpConnection::Options options;
	options.noDelay = true;
	
	return tcpConnection.init(ipAddress, tcpPort, options, [=]()
	{
	#if 1
		// tell the TCP stack to use a specific buffer size. usually the TCP stack is
		// configured to use a rather large buffer size to increase bandwidth. we want
		// to keep latency down however, so we reduce the buffer size here
		
		const int sock_value =
			I2S_2CH_BUFFER_COUNT  * /* N times buffered */
			I2S_2CH_FRAME_COUNT   * /* frame count */
			I2S_2CH_CHANNEL_COUNT * /* stereo */
			sizeof(int16_t) /* sample size */;
		setsockopt(tcpConnection.sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sock_value, sizeof(sock_value));
	#endif

	// todo : strp-laserapp : use writev or similar to send multiple packets to the same Artnet controller
		
		while (tcpConnection.wantsToStop.load() == false)
		{
			// todo : perform disconnection test
			
			// generate some audio data
			
			int16_t data[I2S_2CH_FRAME_COUNT][2];
			
			// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
			if (audioStream.IsOpen_get() == false)
			{
				memset(data, 0, sizeof(data));
			}
			else
			{
				AudioSample samples[I2S_2CH_FRAME_COUNT];
				
				for (int i = audioStream.Provide(I2S_2CH_FRAME_COUNT, samples); i < I2S_2CH_FRAME_COUNT; ++i)
				{
					samples[i].channel[0] = 0;
					samples[i].channel[1] = 0;
				}
				
				const int volume14 = volume.load() * (1 << 14);
				
				for (int i = 0; i < I2S_2CH_FRAME_COUNT; ++i)
				{
					data[i][0] = (samples[i].channel[0] * volume14) >> 14;
					data[i][1] = (samples[i].channel[1] * volume14) >> 14;
				}
			}
			
			if (send(tcpConnection.sock, (const char*)data, sizeof(data), 0) < (ssize_t)sizeof(data))
			{
				LOG_ERR("failed to send data");
				tcpConnection.wantsToStop = true;
			}
		}
	});
}

void Test_TcpToI2S::shut()
{
	LOG_DBG("shutting down TCP connection");
	
	tcpConnection.beginShutdown();
	
	//
	
	audioStream.Close();
	
	//
	
	tcpConnection.waitForShutdown();
	
	LOG_DBG("shutting down TCP connection [done]");
}
