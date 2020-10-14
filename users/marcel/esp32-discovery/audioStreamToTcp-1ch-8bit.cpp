#include "audioStreamToTcp-1ch-8bit.h"

#include "nodeDiscovery.h"

#include "Log.h"

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <sys/socket.h>
#endif

bool Test_TcpToI2SMono8::init(const uint32_t ipAddress, const uint16_t tcpPort, const char * filename)
{
	audioStream.Open(filename, true);
	
	ThreadedTcpConnection::Options options;
	options.noDelay = true;
	
	return tcpConnection.init(ipAddress, tcpPort, options, [=]()
	{
		// tell the TCP stack to use a specific buffer size. usually the TCP stack is
		// configured to use a rather large buffer size to increase bandwidth. we want
		// to keep latency down however, so we reduce the buffer size here
		
		const int sock_value =
			I2S_1CH_8_BUFFER_COUNT  * /* N times buffered */
			I2S_1CH_8_FRAME_COUNT   * /* frame count */
			I2S_1CH_8_CHANNEL_COUNT * /* stereo */
			sizeof(int8_t) /* sample size */;
		setsockopt(tcpConnection.sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sock_value, sizeof(sock_value));
	
		LOG_DBG("frame size: %d", I2S_1CH_8_FRAME_COUNT * sizeof(int8_t));
		
		while (tcpConnection.wantsToStop.load() == false)
		{
			// todo : perform disconnection test
			
			// generate some audio data
			
			int8_t data[I2S_1CH_8_FRAME_COUNT];
			
			// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
			if (audioStream.IsOpen_get() == false)
			{
				memset(data, 0, sizeof(data));
			}
			else
			{
				AudioSample samples[I2S_1CH_8_FRAME_COUNT];
				
				for (int i = audioStream.Provide(I2S_1CH_8_FRAME_COUNT, samples); i < I2S_1CH_8_FRAME_COUNT; ++i)
				{
					samples[i].channel[0] = 0;
					samples[i].channel[1] = 0;
				}
				
				const int volume14 = volume.load() * (1 << 14);
				
				for (int i = 0; i < I2S_1CH_8_FRAME_COUNT; ++i)
				{
					const int value =
						(
							(
								samples[i].channel[0] +
								samples[i].channel[1]
							) * volume14
						) >> (14 + 1);
					
					//Assert(value >= -128 && value <= +127);
					
					data[i] = value;
					
					//Assert(data[i] == value);
				}
			}
			
			if (send(tcpConnection.sock, (const char*)data, sizeof(data), 0) < (ssize_t)sizeof(data))
			{
				LOG_ERR("failed to send data");
				tcpConnection.wantsToStop = true;
			}
		}
	});
	
	return true;
}

void Test_TcpToI2SMono8::shut()
{
	LOG_DBG("shutting down TCP connection");
	
	tcpConnection.beginShutdown();
	
	//
	
	audioStream.Close();
	
	//
	
	tcpConnection.waitForShutdown();
	
	LOG_DBG("shutting down TCP connection [done]");
}

bool Test_TcpToI2SMono8::isActive() const
{
	return tcpConnection.sock != -1;
}
