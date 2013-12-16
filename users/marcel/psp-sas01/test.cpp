#include "stdafx.h"
#include <audiooutput.h>
#include <kernel.h>
#include <libatrac3plus.h>
#include <libsas.h>
#include <stdio.h>
#include <string.h>
#include <utility/utility_module.h>

// sceAudioOutput2: provides double buffered PCM output to speakers
// sceSas: provides voice mixing on the media engine
// sceAtrac: provides AT3 decoding
// note: sceSas & sceAtrac cooperate to decode AT3 data efficiently

SCE_MODULE_INFO( sas1, 0, 1, 1 );

int loadModules();
int unloadModules();

const static int AT3_INIT_SIZE = 1024 * 8;
const static int AT3_DATA_SIZE = 1024 * 32;
const static int OUTPUT_SIZE = 1024;
const static int ADSR = SCE_SAS_ATTACK_VALID | SCE_SAS_DECAY_VALID | SCE_SAS_SUSTAIN_VALID | SCE_SAS_RELEASE_VALID;

static SceUChar8 at3Init[AT3_INIT_SIZE + 2] __attribute__((aligned(64)));
static SceUChar8 at3Data[AT3_DATA_SIZE * 2 + 2] __attribute__((aligned(64)));
static short pcmData[2][OUTPUT_SIZE * 2] __attribute__((aligned(64)));

const static char* fileName = "host0:test5.at3";
const static int voiceIdx = 0;
static SceUID fd = 0;
static SceUID atracId = 0;

int main(int argc, char* argv[])
{
	int retcode = 0;

	if (sceKernelSetCompiledSdkVersion(SCE_DEVKIT_VERSION) < 0)
	{
		printf("failed to set compiled SDK version\n");
		return -1;
	}

	// load modules

	if (loadModules() < 0)
	{
		printf("failed to load modules\n");
		return -1;
	}

	// initialize PCM output

	if (sceAudioOutput2Reserve(OUTPUT_SIZE) < 0)
	{
		printf("failed to create audio output\n");
		return -1;
	}

	// initialize media engine

	if (sceSasInitWithGrain(OUTPUT_SIZE) < 0)
	{
		printf("failed to initialize SAS\n");
		return -1;
	}

	if (sceSasSetOutputmode(SCE_SAS_OUTPUTMODE_STEREO) < 0)
	{
		printf("failed to set SAS output mode to stereo\n");
		return -1;
	}

	// load audio data

	fd = sceIoOpen(fileName, SCE_O_RDONLY, 0);

	if (fd < 0)
	{
		printf("failed to open AT3 file\n");
		return -1;
	}

	printf("opened AT3 file\n");

	const SceSSize size = sceIoRead(fd, at3Init, AT3_INIT_SIZE);

	printf("header read size: %d\n", (int)size);

	// initialize AT3

	printf("initializing AT3\n");

	atracId = sceAtracSetDataAndGetID(at3Init, size);

	if (atracId < 0)
	{
		printf("failed to get atrac ID\n");
		return -1;
	}

	retcode = sceSasSetVoiceATRAC3(voiceIdx, atracId);

	if (retcode < 0)
	{
		printf("failed to assign AT3 ID to SAS voice (%08x)\n", retcode);
		return -1;
	}

	// note: these must be set and valid for the voice to play
	// ADSR = Attack Decay Sustain Release

	sceSasSetADSRmode(voiceIdx, ADSR, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT, SCE_SAS_ADSR_MODE_DIRECT);
	sceSasSetADSR(voiceIdx, ADSR, SCE_SAS_ENVELOPE_RATE_MAX, 0, 0, SCE_SAS_ENVELOPE_RATE_MAX);
	const int volume = SCE_SAS_VOLUME_MAX;
	sceSasSetVolume(voiceIdx, volume, volume, volume, volume);

	if (sceSasSetKeyOn(voiceIdx) < 0)
	{
		printf("failed to enabled SAS voice\n");
		return -1;
	}

	int at3DataIdx = 0;
	int pcmDataIdx = 0;

	while (true)
	{
		retcode = sceSasCore(pcmData[pcmDataIdx]);

		if (retcode < 0)
		{
			printf("failed to mix PCM data (%08x)\n", retcode);
			return -1;
		}
		else
		{
			//printf("mixed PCM data\n");
		}

		if (sceAudioOutput2OutputBlocking(SCE_AUDIO_VOLUME_0dB, pcmData[pcmDataIdx]) < 0)
		{
			printf("failed to output PCM data\n");
			return -1;
		}

		pcmDataIdx = (pcmDataIdx + 1) % 2;

		//

		if (sceSasCheckATRAC3BufferStatus(voiceIdx) > 0)
		{
			SceSSize size = sceIoRead(fd, at3Data + AT3_DATA_SIZE * at3DataIdx, AT3_DATA_SIZE);

			if (sceSasConcatenateATRAC3(
				voiceIdx,
				at3Data + AT3_DATA_SIZE * at3DataIdx,
				size) < 0)
			{
				printf("failed to concatenate AT3 data\n");
			}
			else
			{
				printf("concatenated AT3 data\n");

				at3DataIdx = (at3DataIdx + 1) % 2;
			}
		}
		else
		{
			//printf("no AT3 data needed\n");
		}
	}

	// release AT3 ID

	if (sceSasReleaseATRAC3Id(atracId) < 0)
	{
		printf("failed to release AT3 ID\n");
		return -1;
	}

	// flush PCM data

	if (sceAudioOutput2OutputBlocking(0, 0) < 0)
	{
		printf("failed to flush PCM data\n");
		return -1;
	}

	// exit SAS

	if (sceSasExit() < 0)
	{
		printf("failed to exit SAS\n");
		return -1;
	}

	// stop PCM output

	if (sceAudioOutput2Release() < 0)
	{
		printf("failed to end PCM output\n");
		return -1;
	}

	// unload modules

	if (unloadModules() < 0)
	{
		printf("failed to unload modules\n");
		return -1;
	}

	return 0;
}

int loadModules()
{
	int res;
	res = sceUtilityLoadModule(SCE_UTILITY_MODULE_AV_AVCODEC);
	if (res < 0) return res;
	res = sceUtilityLoadModule(SCE_UTILITY_MODULE_AV_SASCORE);
	if (res < 0) return res;
	res = sceUtilityLoadModule(SCE_UTILITY_MODULE_AV_LIBATRAC3PLUS);
	if (res < 0) return res;
	return 0;
}

int unloadModules()
{
	sceUtilityUnloadModule(SCE_UTILITY_MODULE_AV_LIBATRAC3PLUS);
	sceUtilityUnloadModule(SCE_UTILITY_MODULE_AV_AVCODEC);
	sceUtilityUnloadModule(SCE_UTILITY_MODULE_AV_SASCORE);
	return 0;
}
