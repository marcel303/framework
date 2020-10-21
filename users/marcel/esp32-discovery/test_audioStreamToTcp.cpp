#include "test_audioStreamToTcp.h"

#include "nodeDiscovery.h"
#include "framework.h"
#include "Log.h"

bool Test_AudioStreamToTcp::init(
	const uint32_t ipAddress,
	const uint16_t tcpPort,
	const int numBuffers,
	const int numFramesPerBuffer,
	const int numChannelsPerFrame,
	const bool lowQualityMode,
	const char * filename)
{
	audioStream.Open(filename, true);

	audioStreamToTcp.init(
		ipAddress, tcpPort,
		numBuffers,
		numFramesPerBuffer,
		numChannelsPerFrame,
		AudioStreamToTcp::kSampleFormat_S16,
		lowQualityMode
			? AudioStreamToTcp::kSampleFormat_S8
			: AudioStreamToTcp::kSampleFormat_S16,
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
					for (int c = 0; c < numChannels; ++c)
					{
						*out_samples16++ = samples[i].channel[c & 1];
					}
				}
			}
		});
	
	return true;
}

void Test_AudioStreamToTcp::shut()
{
	audioStreamToTcp.beginShutdown();
	audioStreamToTcp.waitForShutdown();
	
	audioStream.Close();
}

void Test_AudioStreamToTcp::tick()
{
	audioStreamToTcp.volume.store(mouse.x / 800.f);
}

bool Test_AudioStreamToTcp::isActive() const
{
	return audioStreamToTcp.tcpConnection.sock != -1;
}
