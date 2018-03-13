#include "audio.h"
#include "audioTypes.h"
#include "framework.h"
#include "objects/paobject.h"
#include "Path.h"

#include <ctime>

#define CHANNEL_COUNT 2

#define PLAYER_COUNT CHANNEL_COUNT

#define INSTRUCTION_CHANNEL 0

#define RAMP_DURATION_IN_SAMPLES 10000

#define PLAYER_WAIT_MIN .5f
#define PLAYER_WAIT_MAX 1.f

const int GFX_SX = 640;
const int GFX_SY = 480;

struct Example
{
	std::string filename;
	
	SoundData * soundData = nullptr;
};

std::vector<Example> s_examples;

static int s_example = -1;

static int s_examplePosition = 0;

struct Recording
{
	std::vector<float> samples;
};

static AudioMutex s_mutex;

static Recording s_recording;

static std::vector<Recording> s_recordings;

static bool s_record = false;

struct Player
{
	enum State
	{
		kState_Idle,
		kState_Wait,
		kState_Play
	};
	
	State state = kState_Idle;
	float stateTime = 0.f;
	
	int channel = 0;
	int recording = 0;
	int position = 0;
	
	float ramp = 0.f;
};

static Player s_players[PLAYER_COUNT];

struct MyPortAudioHandler : PortAudioHandler
{
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		s_mutex.lock();
		{
			memset(outputBuffer, 0, framesPerBuffer * CHANNEL_COUNT * sizeof(float));
			
			float * dst = (float*)outputBuffer;
			
			for (int p = 0; p < PLAYER_COUNT; ++p)
			{
				auto & player = s_players[p];
				
				if (player.state == Player::kState_Idle)
				{
					if (!s_recordings.empty())
					{
						player.state = Player::kState_Wait;
						player.stateTime = random(PLAYER_WAIT_MIN, PLAYER_WAIT_MAX);
						break;
					}
				}
				else if (player.state == Player::kState_Wait)
				{
					player.stateTime = std::max(0.f, player.stateTime - AUDIO_UPDATE_SIZE / float(SAMPLE_RATE));
					
					if (player.stateTime == 0.f)
					{
						player.channel = p;
						player.recording = rand() % s_recordings.size();
						player.position = 0;
						player.ramp = 0.f;
						
						player.state = Player::kState_Play;
						break;
					}
				}
				else if (player.state == Player::kState_Play)
				{
					auto & recording = s_recordings[player.recording];
					
					for (int i = 0; i < framesPerBuffer; ++i)
					{
						if (player.position == recording.samples.size())
						{
							player.state = Player::kState_Idle;
							break;
						}
						
						if (player.position + RAMP_DURATION_IN_SAMPLES < recording.samples.size())
						{
							player.ramp = std::min(1.f, player.ramp + 1.f / RAMP_DURATION_IN_SAMPLES);
						}
						else
						{
							player.ramp = std::max(0.f, player.ramp - 1.f / RAMP_DURATION_IN_SAMPLES);
						}
						
						const float value = recording.samples[player.position] * player.ramp;
						
						dst[i * CHANNEL_COUNT + player.channel] += value;
						
						player.position++;
					}
				}
			}
			
			if (s_example != -1)
			{
				auto & example = s_examples[s_example];
				
				for (int i = 0; i < framesPerBuffer; ++i)
				{
					if (s_examplePosition == example.soundData->sampleCount)
						break;
					
					Assert(s_examplePosition < example.soundData->sampleCount);
					
					const short * src = (short*)example.soundData->sampleData;
					
					const float value = src[s_examplePosition * 2] / float(1 << 15);
					
					dst[i * CHANNEL_COUNT + INSTRUCTION_CHANNEL] += value;
					
					s_examplePosition++;
				}
			}
			
			if (s_record)
			{
				if (numInputChannels > 0)
				{
					float * src = (float*)inputBuffer;
					
					for (int i = 0; i < framesPerBuffer; ++i)
					{
						const float value = src[i * numInputChannels];
						
						s_recording.samples.push_back(value);
					}
				}
			}
		}
		s_mutex.unlock();
	}
};

#include "StringEx.h"

static void handleError(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", text, SDL_GetKeyboardFocus());
}

#if LINUX
	#include <portaudio.h>
#else
	#include <portaudio/portaudio.h>
#endif

#include "../../../libparticle/ui.h"

static bool doPaMenu(const bool tick, const bool draw, const float dt, int & inputDeviceIndex, int & outputDeviceIndex)
{
	bool result = false;
	
	pushMenu("pa");
	{
		const int numDevices = Pa_GetDeviceCount();
		
		std::vector<EnumValue> inputDevices;
		std::vector<EnumValue> outputDevices;
		
		for (int i = 0; i < numDevices; ++i)
		{
			const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
			
			if (deviceInfo->maxInputChannels > 0)
			{
				EnumValue e;
				e.name = deviceInfo->name;
				e.value = i;
				
				inputDevices.push_back(e);
			}
			
			if (deviceInfo->maxOutputChannels > 0)
			{
				EnumValue e;
				e.name = deviceInfo->name;
				e.value = i;
				
				outputDevices.push_back(e);
			}
		}
		
		if (tick)
		{
			if (inputDeviceIndex == paNoDevice && inputDevices.empty() == false)
				inputDeviceIndex = inputDevices.front().value;
			if (outputDeviceIndex == paNoDevice && outputDevices.empty() == false)
				outputDeviceIndex = outputDevices.front().value;
		}
		
		doDropdown(inputDeviceIndex, "Input", inputDevices);
		doDropdown(outputDeviceIndex, "Output", outputDevices);
		
		doBreak();
		
		if (doButton("OK"))
		{
			result = true;
		}
	}
	popMenu();
	
	return result;
}

enum State
{
	kState_Idle,
	kState_Instruction,
	kState_InstructionPlay,
	kState_Recording
};

static State s_state = kState_Idle;

static float s_stateTimer = 0.f;

static void beginRecording()
{
	s_mutex.lock();
	{
		s_record = true;
	}
	s_mutex.unlock();
}

static void endRecording()
{
	Recording r;

	s_mutex.lock();
	{
		s_record = false;
		
		s_recording.samples.swap(r.samples);
	}
	s_mutex.unlock();

	s_recordings.push_back(r);

	if (r.samples.size() > 0)
	{
		char filename[128];
		
		time_t t = time(0);
		struct tm * now = localtime(&t);
		
		sprintf(filename, "recording %04d-%02d-%02d %02d-%02d-%02d.pcm",
			1900 + now->tm_year,
			1 + now->tm_mon,
			now->tm_mday,
			now->tm_hour,
			now->tm_min,
			now->tm_sec);
		
		FILE * file = fopen(filename, "wb");
		
		if (file != nullptr)
		{
			fwrite(&r.samples[0], sizeof(float), r.samples.size(), file);
			
			fclose(file);
		}
	}
}

int main(int argc, char * argv[])
{
	changeDirectory(SDL_GetBasePath());
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		handleError("Failed to initialize system");
		return -1;
	}
	
	initUi();
	
	s_mutex.init();
	
	int inputDeviceIndex = paNoDevice;
	int outputDeviceIndex = paNoDevice;
	
	UiState uiState;
	uiState.sx = 400;
	uiState.x = (GFX_SX - uiState.sx) / 2;
	uiState.y = GFX_SY / 2;
	
	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();
		
		framework.beginDraw(100, 100, 100, 0);
		{
			makeActive(&uiState, true, true);
			
			if (doPaMenu(true, true, framework.timeStep, inputDeviceIndex, outputDeviceIndex))
				break;
		}
		framework.endDraw();
	}
	
	if (inputDeviceIndex == paNoDevice || outputDeviceIndex == paNoDevice)
	{
		handleError("No input and/or output device selected");
		return -1;
	}
	
	MyPortAudioHandler paHandler;
	
	PortAudioObject pa;
	
	if (!pa.init(SAMPLE_RATE, CHANNEL_COUNT, 1, AUDIO_UPDATE_SIZE, &paHandler, inputDeviceIndex, outputDeviceIndex))
	{
		handleError("Failed to initialize audio input and/or output device");
		return -1;
	}
	
	auto exampleFilenames = listFiles(".", false);
	
	for (auto & filename : exampleFilenames)
	{
		if (Path::GetExtension(filename, true) != "ogg")
			continue;
		
		Example e;
		e.filename = filename;
		e.soundData = loadSound(filename.c_str());
		
		if (e.soundData == nullptr)
		{
			handleError("Failed to load instructional audio file (%s)", filename.c_str());
			continue;
		}
		
		if (e.soundData->channelSize != 2 || e.soundData->channelCount != 2)
		{
			handleError("Failed to load instructional audio file (%s). Audio must be encoded as 16-bit, mono", filename.c_str());
			continue;
		}
		
		s_mutex.lock();
		{
			s_examples.push_back(e);
		}
		s_mutex.unlock();
	}
	
	if (s_examples.empty())
	{
		handleError("Failed to find any example/instructional audio files");
		return -1;
	}
	
	for (;;)
	{
		framework.waitForEvents = s_state == kState_Idle;
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
		
		// todo : button press ? start recording
		
		if (s_state == kState_Idle)
		{
			if (keyboard.wentDown(SDLK_SPACE))
			{
				s_state = kState_Instruction;
				s_stateTimer = 4.f;
				
				s_mutex.lock();
				{
					s_example = rand() % s_examples.size();
					
					s_examplePosition = 0;
				}
				s_mutex.unlock();
			}
		}
		else if (s_state == kState_Instruction)
		{
			s_stateTimer = std::max(0.f, s_stateTimer - dt);
			
			if (s_stateTimer == 0.f)
			{
				s_state = kState_InstructionPlay;
				s_stateTimer = 4.f;
			}
		}
		else if (s_state == kState_InstructionPlay)
		{
			s_stateTimer = std::max(0.f, s_stateTimer - dt);
			
			if (s_stateTimer == 0.f)
			{
				s_state = kState_Recording;
				s_stateTimer = 4.f;
				
				beginRecording();
			}
		}
		else if (s_state == kState_Recording)
		{
			s_stateTimer = std::max(0.f, s_stateTimer - dt);
			
			if (s_stateTimer == 0.f)
			{
				endRecording();
				
				s_mutex.unlock();
				{
					s_example = -1;
				}
				s_mutex.unlock();
				
				s_state = kState_Idle;
				s_stateTimer = 4.f;
			}
		}
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (s_record == false)
			{
				beginRecording();
			}
			else
			{
				endRecording();
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			
			if (s_state == kState_Idle)
			{
				drawText(GFX_SX/2, GFX_SY/3, 16, 0, 0, "Hello");
			}
			else if (s_state == kState_Instruction)
			{
				drawText(GFX_SX/2, GFX_SY/3, 16, 0, 0, "Instruction");
			}
			else if (s_state == kState_InstructionPlay)
			{
				drawText(GFX_SX/2, GFX_SY/3, 16, 0, 0, "Instruction, Play Sound");
			}
			else if (s_state == kState_Recording)
			{
				drawText(GFX_SX/2, GFX_SY/3, 16, 0, 0, "Recording");
			}
			
			if (s_record)
			{
				drawText(GFX_SX/2, GFX_SY/2 + 40, 16, 0, 0, "recording in progress..");
			}
			
			int index = 0;
			
			for (auto & r : s_recordings)
			{
				drawText(GFX_SX/2, GFX_SY/2 + 60 + index * 20, 16, 0, 0, "recording. length = %d", r.samples.size());
				
				index++;
			}
		}
		framework.endDraw();
	}
	
	return 0;
}
