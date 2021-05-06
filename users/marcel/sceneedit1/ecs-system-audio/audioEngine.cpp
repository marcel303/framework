#include "audioEngine.h"

#include "audioEmitterToStereoOutputBuffer.h"
#include "audiooutput-hd/AudioOutputHD_Native.h"

#include "audioEmitterComponent.h"
#include "reverbZoneComponent.h"

#include "framework.h" // framework.resolveResourcePath

#include "binaural.h"
#include "binaural_oalsoft.h"

#include <mutex>
#include <vector>

const int kAudioBufferSize = 256; // todo : configure somewhere
const int kAudioFrameRate = 48000; // note : needs to be 48kHz for OculusVR, and for binaulization due to HRIR sample sets being authored for 48kHz
	
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
		
	class MyAudioStream : public AudioStreamHD
	{
	public:
		std::mutex mutex;
		
		AudioEngine_Binaural * audioEngine = nullptr;
		
		virtual int Provide(const ProvideInfo & provideInfo, const StreamInfo & streamInfo) override
		{
			audioEngine->onAudioThreadProcess();
			
			g_audioEmitterComponentMgr.onAudioThreadProcess();
			g_reverbZoneComponentMgr.onAudioThreadProcess();
			
			audioEmitterToStereoOutputBuffer(
				audioEngine->hrirSampleSet,
				audioEngine->worldToView,
				provideInfo.outputSamples[0],
				provideInfo.outputSamples[1],
				provideInfo.numFrames);
			
			return 2;
		}
	};
	
	AudioOutputHD_Native audioOutput;
	
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
	audioOutput.Initialize(0, 2, kAudioFrameRate, kAudioBufferSize);
	
	audioStream.audioEngine = this;
	
	loadHrirSampleSet("ecs-system-audio/binaural/Default HRTF.mhr");
}

void AudioEngine_Binaural::shut()
{
	audioOutput.Shutdown();
}

void AudioEngine_Binaural::start()
{
	g_audioEmitterComponentMgr.onAudioThreadBegin(
		audioOutput.FrameRate_get(),
		audioOutput.BufferSize_get());
		
	g_reverbZoneComponentMgr.onAudioThreadBegin(
		audioOutput.FrameRate_get(),
		audioOutput.BufferSize_get());
	
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
