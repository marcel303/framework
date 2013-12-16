#include <audiooutput.h>
#include <kernel.h>
#include <libatrac3plus.h>
#include <libsas.h>
#include <stdio.h>
#include "Calc.h"
#include "Debugging.h"
#include "FixedSizeString.h"
#include "SoundPlayer_Psp_AT3.h"

// todo: scePowerSetClockFrequency ?
// todo: move sas init

// sceAudioOutput2: provides double buffered PCM output to speakers
// sceSas: provides voice mixing on the media engine
// sceAtrac: provides AT3 decoding
// note: sceSas & sceAtrac cooperate to decode AT3 data efficiently

const static int AT3_INIT_SIZE = 1024 * 8;
const static int AT3_DATA_SIZE = 1024 * 32;
const static int OUTPUT_SIZE = 1024;
const static int ADSR = SCE_SAS_ATTACK_VALID | SCE_SAS_DECAY_VALID | SCE_SAS_SUSTAIN_VALID | SCE_SAS_RELEASE_VALID;

static SceUChar8 at3Init[AT3_INIT_SIZE + 2] __attribute__((aligned(64)));
static SceUChar8 at3Data[AT3_DATA_SIZE * 2 + 2] __attribute__((aligned(64)));
static short pcmData[2][OUTPUT_SIZE * 2] __attribute__((aligned(64)));

const static int voiceIdx = 31;
static SceUID fd = 0;
static SceUID atracId = 0;
static SceUID threadId = 0;
static int volume = SCE_SAS_VOLUME_MAX;
static FixedSizeString<128> at3FileName;
static bool at3Loop = false;
static bool at3IsEnabled = true;

static volatile bool isPlaying = false;
static volatile bool mustStop = false;
static volatile bool isDone = false;

static int ThreadStart(SceSize argSize, void* arg);

#define PLAYTHREAD_STACKSIZE (1024 * 32)
#define PLAYTHREAD_PRIORITY (SCE_KERNEL_USER_HIGHEST_PRIORITY + 2)

void At3Begin(const char* fileName, bool loop)
{
	LOG_INF("initiating AT3 playback", 0);

	Assert(fd == 0);
	Assert(atracId == 0);
	Assert(threadId == 0);

	int retcode = 0;

	at3FileName = fileName;

	// load audio data

	fd = sceIoOpen(fileName, SCE_O_RDONLY, 0);

	if (fd < 0)
	{
		LOG_DBG("failed to open AT3 file", 0);
		return;
	}

	LOG_DBG("opened AT3 file", 0);

	const SceSSize size = sceIoRead(fd, at3Init, AT3_INIT_SIZE);

	LOG_DBG("header read size: %d", (int)size);

	// initialize AT3

	LOG_DBG("initializing AT3", 0);

	atracId = sceAtracSetDataAndGetID(at3Init, size);

	if (atracId < 0)
	{
		LOG_DBG("failed to get atrac ID", 0);
		return;
	}

	retcode = sceSasSetVoiceATRAC3(voiceIdx, atracId);

	if (retcode < 0)
	{
		LOG_DBG("failed to assign AT3 ID to SAS voice (%08x)", retcode);
		return;
	}

	// note: these must be set and valid for the voice to play
	// ADSR = Attack Decay Sustain Release

	sceSasSetADSRmode(voiceIdx, ADSR, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT);
	sceSasSetADSR(voiceIdx, ADSR, SCE_SAS_ENVELOPE_RATE_MAX, 0, 0, SCE_SAS_ENVELOPE_RATE_MAX);
	sceSasSetVolume(voiceIdx, volume, volume, volume, volume);

	if (sceSasSetKeyOn(voiceIdx) < 0)
	{
		LOG_DBG("failed to enabled SAS voice", 0);
		return;
	}

	at3Loop = loop;
	isDone = false;
	mustStop = false;
	isPlaying = true;

	threadId = sceKernelCreateThread(
		"ATRAC3plus play thread",
		ThreadStart,
		PLAYTHREAD_PRIORITY,
		PLAYTHREAD_STACKSIZE, 0, NULL);

	if (threadId < 0)
	{
		LOG_DBG("failed to create thread", 0);
		return;
	}

	if (sceKernelStartThread(threadId, 0, 0) < 0)
	{
		LOG_DBG("failed to start thread", 0);
		return;
	}

	LOG_INF("initiating AT3 playback [done]", 0);
}

void At3SetEnabled(bool enabled)
{
	at3IsEnabled = enabled;
}

void At3SetVolume(float _volume)
{
	volume = SCE_SAS_VOLUME_MAX * Calc::Saturate(_volume);

	sceSasSetVolume(voiceIdx, volume, volume, volume, volume);
}

static int ThreadStart(SceSize argSize, void* arg)
{
	int retcode = 0;

	int at3DataIdx = 0;
	int pcmDataIdx = 0;

	bool at3IsEnabled_Current = true;

	while (mustStop == false)
	{
		retcode = sceSasCore(pcmData[pcmDataIdx]);

		if (retcode < 0)
		{
			LOG_DBG("failed to mix PCM data (%08x)", retcode);
			break;
		}
		else
		{
			//LOG_DBG("mixed PCM data", 0);
		}

		if (sceAudioOutput2OutputBlocking(SCE_AUDIO_VOLUME_0dB, pcmData[pcmDataIdx]) < 0)
		{
			LOG_DBG("failed to output PCM data", 0);
			break;
		}

		pcmDataIdx = (pcmDataIdx + 1) % 2;

		//

		if (at3IsEnabled != at3IsEnabled_Current)
		{
			LOG_DBG("setting AT3 enabled flag to %d", at3IsEnabled ? 1 : 0);
			sceSasSetPause(voiceIdx, !at3IsEnabled);
			at3IsEnabled_Current = at3IsEnabled;
		}

		if (sceSasCheckATRAC3BufferStatus(voiceIdx) > 0)
		{
			SceSSize size = sceIoRead(fd, at3Data + AT3_DATA_SIZE * at3DataIdx, AT3_DATA_SIZE);

			if (size == 0)
			{
				isDone = true;
			}
			else
			{
				if (sceSasConcatenateATRAC3(
					voiceIdx,
					at3Data + AT3_DATA_SIZE * at3DataIdx,
					size) < 0)
				{
					LOG_DBG("failed to concatenate AT3 data", 0);
					break;
				}
				else
				{
					LOG_DBG("concatenated AT3 data", 0);

					at3DataIdx = (at3DataIdx + 1) % 2;
				}
			}
		}
		else
		{
			//LOG_DBG("no AT3 data needed", 0);
		}
	}

	return 0;
}

void At3End()
{
	if (isPlaying == false)
	{
		LOG_DBG("not playing", 0);
		return;
	}

	LOG_INF("shutting down AT3 playback", 0);

	mustStop = true;

	if (sceKernelWaitThreadEnd(threadId, SCE_NULL) < 0)
		LOG_DBG("failed to wait for thread end", 0);
	if (sceKernelDeleteThread(threadId) < 0)
		LOG_DBG("failed to delete thread", 0);
	threadId = 0;

	isPlaying = false;

	// release AT3 ID

	if (sceSasReleaseATRAC3Id(atracId) < 0)
		LOG_DBG("failed to release AT3 ID", 0);
	atracId = 0;

	// close file

	sceIoClose(fd);
	fd = 0;

	LOG_INF("shutting down AT3 playback [done]", 0);
}

void At3CheckLoop()
{
	if (isPlaying && isDone)
	{
		LOG_INF("initiating AT3 loop", 0);

		At3End();
		At3Begin(at3FileName.c_str(), at3Loop);
	}
}
