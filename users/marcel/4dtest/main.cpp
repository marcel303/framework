#include "framework.h"
#include "Log.h"
#include "../libparticle/ui.h"
#include "portaudio/portaudio.h"

const int GFX_SX = 400;
const int GFX_SY = 400;

#define SAMPLERATE 44100

enum AppState
{
	kAppState_Init,
	kAppState_InitFailed,
	kAppState_InitDone
};

struct AppData
{
	AppState state = kAppState_Init;
	
	int inputDeviceIndex = paNoDevice;
	int outputDeviceIndex = paNoDevice;
	
	PaStream * stream = nullptr;
	
	bool muteAudio = false;
};

static AppData appData;

static int portaudioCallback(
	const void * inputBuffer,
	void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	float * inputValues = (float*)inputBuffer;
	float * outputValues = (float*)outputBuffer;
	
	if (appData.muteAudio)
	{
		memset(outputValues, 0, framesPerBuffer * 2 * sizeof(float));
	}
	else
	{
		memcpy(outputValues, inputValues, framesPerBuffer * 2 * sizeof(float));
	}
	
	return paContinue;
}

static bool paInit(const int inputDeviceIndex, const int numInputChannels, const int outputDeviceIndex, const int numOutputChannels, const int sampleRate, const int bufferSize)
{
	PaError err = paNoError;
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = outputDeviceIndex;
	
	if (outputParameters.device == paNoDevice)
	{
		LOG_ERR("portaudio: failed to find output device", 0);
		return false;
	}
	
	outputParameters.channelCount = numOutputChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//
	
	PaStreamParameters inputParameters;
	memset(&inputParameters, 0, sizeof(inputParameters));
	
	inputParameters.device = inputDeviceIndex;
	
	if (inputParameters.device == paNoDevice)
	{
		LOG_ERR("portaudio: failed to find input device", 0);
		return false;
	}
	
	inputParameters.channelCount = numInputChannels;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&appData.stream, numInputChannels == 0 ? nullptr : &inputParameters, numOutputChannels == 0 ? nullptr : &outputParameters, sampleRate, bufferSize, paDitherOff, portaudioCallback, nullptr)) != paNoError)
	{
		LOG_ERR("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(appData.stream)) != paNoError)
	{
		LOG_ERR("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}

	return true;
}

static void doPaMenu(const bool tick, const bool draw, const float dt)
{
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
			if (appData.inputDeviceIndex == -1 && inputDevices.empty() == false)
				appData.inputDeviceIndex = inputDevices.front().value;
			if (appData.outputDeviceIndex == -1 && outputDevices.empty() == false)
				appData.outputDeviceIndex = outputDevices.front().value;
		}
		
		doDropdown(appData.inputDeviceIndex, "Input", inputDevices);
		doDropdown(appData.outputDeviceIndex, "Output", outputDevices);
		
		doBreak();
		
		if (doButton("OK"))
		{
			// todo : initialize PortAudio
			
			if (paInit(appData.inputDeviceIndex, 2, appData.outputDeviceIndex, 2, SAMPLERATE, 256))
			{
				appData.state = kAppState_InitDone;
			}
			else
			{
				appData.state = kAppState_InitFailed;
			}
		}
	}
	popMenu();
}

static void doStreamMenu(const bool tick, const bool draw, const float dt)
{
	pushMenu("stream");
	{
		doCheckBox(appData.muteAudio, "Mute", false);
	}
	popMenu();
}

int main(int argc, char * argv[])
{
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		PaError err;
		
		if ((err = Pa_Initialize()) != paNoError)
		{
			LOG_ERR("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
			return false;
		}
		
		LOG_DBG("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
		
		UiState paUiState;
		paUiState.sx = GFX_SX/2;
		paUiState.x = (GFX_SX - paUiState.sx) / 2;
		paUiState.y = 100;
		
		UiState streamUiState;
		streamUiState.sx = GFX_SX/2;
		streamUiState.x = (GFX_SX - streamUiState.sx) / 2;
		streamUiState.y = 100;
		
		do
		{
			framework.process();
			
			if (appData.state == kAppState_Init)
			{
				makeActive(&paUiState, true, false);
				doPaMenu(true, false, framework.timeStep);
			}
			else if (appData.state == kAppState_InitFailed)
			{
				//
			}
			else if (appData.state == kAppState_InitDone)
			{
				makeActive(&streamUiState, true, false);
				doStreamMenu(true, false, framework.timeStep);
			}
		
			framework.beginDraw(200, 200, 200, 0);
			{
				setFont("calibri.ttf");
				
				if (appData.state == kAppState_Init)
				{
					makeActive(&paUiState, false, true);
					doPaMenu(false, true, framework.timeStep);
				}
				else if (appData.state == kAppState_InitFailed)
				{
					setColor(colorWhite);
					drawText(GFX_SX/2, GFX_SY/2, 24, 0, 0, "Init failed!");
				}
				else if (appData.state == kAppState_InitDone)
				{
					setColor(colorWhite);
					drawText(GFX_SX/2, GFX_SY/2, 24, 0, 0, "Init OK");
					
					makeActive(&streamUiState, false, true);
					doStreamMenu(false, true, framework.timeStep);
				}
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_ESCAPE));
		
		framework.shutdown();
	}
	
	return 0;
}
