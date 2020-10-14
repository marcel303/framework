#include "audioStreamToTcp-4ch-16bit.h"

#include "nodeDiscovery.h"
#include "framework.h"
#include "Log.h"

bool Test_TcpToI2SQuad::init(const uint32_t ipAddress, const uint16_t tcpPort, const char * filename)
{
	audioStream.Open(filename, true);

	audioStreamToTcp.init(
		ipAddress, tcpPort,
		I2S_4CH_BUFFER_COUNT,
		I2S_4CH_FRAME_COUNT,
		I2S_4CH_CHANNEL_COUNT,
		AudioStreamToTcp::kSampleFormat_S16,
		[this](void * __restrict out_samples, const int numFrames, const int numChannels)
		{
			// generate some audio data
			
			// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
			if (audioStream.IsOpen_get() == false)
			{
				memset(out_samples, 0, numFrames * numChannels * sizeof(int16_t));
			}
			else
			{
				AudioSample samples[I2S_4CH_FRAME_COUNT];
				
				for (int i = audioStream.Provide(numFrames, samples); i < numFrames; ++i)
				{
					samples[i].channel[0] = 0;
					samples[i].channel[1] = 0;
				}
				
				int16_t * __restrict out_samples16 = (int16_t*)out_samples;
				
				for (int i = 0; i < numFrames; ++i)
				{
					out_samples16[i * 4 + 0] = samples[i].channel[0];
					out_samples16[i * 4 + 1] = samples[i].channel[1];
					out_samples16[i * 4 + 2] = samples[i].channel[0];
					out_samples16[i * 4 + 3] = samples[i].channel[1];
				}
			}
		});
	
	return true;
}

void Test_TcpToI2SQuad::shut()
{
	audioStreamToTcp.beginShutdown();
	audioStreamToTcp.waitForShutdown();
	
	audioStream.Close();
}

void Test_TcpToI2SQuad::tick()
{
	audioStreamToTcp.volume.store(mouse.x / 800.f);
}

bool Test_TcpToI2SQuad::isActive() const
{
	return audioStreamToTcp.tcpConnection.sock != -1;
}
