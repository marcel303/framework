#include "audio.h"
#include "audiostream/AudioOutput.h"
#include "framework.h"

static SDL_Thread * g_audioThread = nullptr;
static volatile bool g_stopAudioThread = false;
static AudioOutput_OpenAL * g_audioOutput = nullptr;
static AudioStreamEx * g_audioStream = nullptr;
static uint32_t g_audioUpdateEvent = -1;

bool g_wantsAudioPlayback = false;

static int SDLCALL ExecuteAudioThread(void * arg)
{
	while (!g_stopAudioThread)
	{
		if (g_wantsAudioPlayback && !g_audioOutput->IsPlaying_get())
			g_audioOutput->Play();
		if (!g_wantsAudioPlayback && g_audioOutput->IsPlaying_get())
			g_audioOutput->Stop();

		g_audioOutput->Update(g_audioStream);
		SDL_Delay(10);

		if (g_audioOutput->IsPlaying_get())
		{
			SDL_Event e;
			memset(&e, 0, sizeof (e));
			e.type = g_audioUpdateEvent;
			SDL_PushEvent(&e);
		}
	}

	return 0;
}

void openAudio(AudioStreamEx * audioStream)
{
	Assert(g_audioStream == nullptr);
	Assert(audioStream != nullptr);
	g_audioStream = audioStream;

	Assert(g_audioOutput == nullptr);
	g_audioOutput = new AudioOutput_OpenAL();
	g_audioOutput->Initialize(2, g_audioStream->GetSampleRate(), 1 << 12); // todo : sample rate;

	Assert(g_audioThread == nullptr);
	Assert(!g_stopAudioThread);
	g_audioThread = SDL_CreateThread(ExecuteAudioThread, "AudioThread", nullptr);
}

void closeAudio()
{
	if (g_audioThread != nullptr)
	{
		g_stopAudioThread = true;
		SDL_WaitThread(g_audioThread, nullptr);
		g_audioThread = nullptr;
		g_stopAudioThread = false;
	}

	if (g_audioOutput != nullptr)
	{
		g_audioOutput->Stop();
		g_audioOutput->Shutdown();
		delete g_audioOutput;
		g_audioOutput = nullptr;
	}

	if (g_audioStream != nullptr)
	{
		g_audioStream = nullptr;
	}
}
