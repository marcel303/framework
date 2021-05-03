#include "audioEngine.h"

#include "audioEmitterToStereoOutputBuffer.h"
#include "audiooutput/AudioOutput_Native.h"

#include "audioEmitterComponent.h"
#include "reverbZoneComponent.h"

#include "framework.h" // framework.resolveResourcePath

#include "binaural.h"
#include "binaural_oalsoft.h"

#include <mutex>
#include <vector>

const int kAudioBufferSize = 256; // todo : configure somewhere
const int kAudioFrameRate = 44100;
	
struct AudioEngine_Binaural : AudioEngineBase
{
	enum CommandType
	{
		kCommandType_UpdateHrirSampleSet,
		kCommandType_UpdateListenerTransform
	};
	
	struct Command
	{
		CommandType type;
		
		struct
		{
			binaural::HRIRSampleSet * hrirSampleSet = nullptr;
		} updateHrirSampleSet;
		
		struct
		{
			Mat4x4 worldToView;
		} updateListenerTransform;
	};
	
	std::vector<Command> commands;
	std::mutex commands_mutex;
	
	binaural::HRIRSampleSet * hrirSampleSet = nullptr;
	
	Mat4x4 worldToView = Mat4x4(true);
		
	class MyAudioStream : public AudioStream
	{
	public:
		std::mutex mutex;
		
		AudioEngine_Binaural * audioEngine = nullptr;
		
		virtual int Provide(int numSamples, AudioSample * __restrict samples) override
		{
			audioEngine->onAudioThreadProcess();
			
			g_audioEmitterComponentMgr.onAudioThreadProcess();
			g_reverbZoneComponentMgr.onAudioThreadProcess();
			
			float outputBufferL[numSamples];
			float outputBufferR[numSamples];
			audioEmitterToStereoOutputBuffer(
				audioEngine->hrirSampleSet,
				audioEngine->worldToView,
				outputBufferL,
				outputBufferR,
				numSamples);
			
			const float scale = ((1 << 15) - 1);
			
			for (int i = 0; i < numSamples; ++i)
			{
				float l = outputBufferL[i];
				float r = outputBufferR[i];
				
				// apply a sigmoid function to ensure the values are between -1 and +1
				l = l / (1.f + sqrtf(l * l));
				r = r / (1.f + sqrtf(r * r));
				
				samples[i].channel[0] = int(l * scale);
				samples[i].channel[1] = int(l * scale);
			}
			
			return numSamples;
		}
	};
	
	AudioOutput_Native audioOutput;
	
	MyAudioStream audioStream;
	
	virtual void init() override;
	virtual void shut() override;
	
	virtual void start() override;
	virtual void stop() override;
	
	virtual void setListenerTransform(const Mat4x4 & worldToView) override;
	
	void loadHrirSampleSet(const char * path);
	
	void onAudioThreadProcess();
};
	
void AudioEngine_Binaural::init()
{
	audioOutput.Initialize(2, kAudioFrameRate, kAudioBufferSize);
	
	audioStream.audioEngine = this;
	
	loadHrirSampleSet("ecs-system-audio/binaural/Default HRTF.mhr");
}

void AudioEngine_Binaural::shut()
{
	audioOutput.Shutdown();
}

void AudioEngine_Binaural::start()
{
	g_audioEmitterComponentMgr.onAudioThreadBegin(kAudioFrameRate, kAudioBufferSize); // todo : use actual values from audio output
	g_reverbZoneComponentMgr.onAudioThreadBegin(kAudioFrameRate, kAudioBufferSize);
	
	audioOutput.Play(&audioStream);
}

void AudioEngine_Binaural::stop()
{
	audioOutput.Stop();
	
	g_reverbZoneComponentMgr.onAudioThreadEnd();
	g_audioEmitterComponentMgr.onAudioThreadEnd();
}

void AudioEngine_Binaural::setListenerTransform(const Mat4x4 & worldToView)
{
	// queue command to update the listener transform
	
	commands_mutex.lock();
	{
		Command command;
		command.type = kCommandType_UpdateListenerTransform;
		command.updateListenerTransform.worldToView = worldToView;
		
		commands.push_back(command);
	}
	commands_mutex.unlock();
}

void AudioEngine_Binaural::loadHrirSampleSet(const char * path)
{
	auto * resolvedPath = framework.resolveResourcePath(path);
	
	auto * newHrirSampleSet = new binaural::HRIRSampleSet();
	
	if (binaural::loadHRIRSampleSet_Oalsoft(resolvedPath, *newHrirSampleSet) == false)
	{
		delete newHrirSampleSet;
		newHrirSampleSet = nullptr;
	}
	else
	{
		newHrirSampleSet->finalize();
		
		// queue command to update the HRIR sample set
		
		commands_mutex.lock();
		{
			Command command;
			command.type = kCommandType_UpdateHrirSampleSet;
			command.updateHrirSampleSet.hrirSampleSet = newHrirSampleSet;
			
			commands.push_back(command);
		}
		commands_mutex.unlock();
	}
}

void AudioEngine_Binaural::onAudioThreadProcess()
{
	commands_mutex.lock();
	{
		for (auto & command : commands)
		{
			switch (command.type)
			{
			case kCommandType_UpdateHrirSampleSet:
				hrirSampleSet = command.updateHrirSampleSet.hrirSampleSet;
				break;
			
			case kCommandType_UpdateListenerTransform:
				worldToView = command.updateListenerTransform.worldToView;
				break;
			}
		}
		
		commands.clear();
	}
	commands_mutex.unlock();
}

//

AudioEngineBase * createAudioEngine()
{
	return new AudioEngine_Binaural();
}
