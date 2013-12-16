#include <audiooutput.h>
#include <kernel.h>
#include <libsas.h>
#include <utility/utility_module.h>
#include "Debugging.h"
#include "psp_prxload.h"

// todo: move SAS/audio-out code here

const static int OUTPUT_SIZE = 1024;

int mix_loadModule()
{
	LOG_INF("loading PSP mixer modules", 0);

	int result = 0;

	result = sceUtilityLoadModule(SCE_UTILITY_MODULE_AV_AVCODEC);

	if (result < 0) 
	{
		LOG_ERR("failed to load avcodec module", 0);
		return result;
	}

	//

	result = sceUtilityLoadModule(SCE_UTILITY_MODULE_AV_SASCORE);

	if (result < 0) 
	{
		LOG_ERR("failed to load sascore module", 0);
		return result;
	}

	//

	result = sceUtilityLoadModule(SCE_UTILITY_MODULE_AV_LIBATRAC3PLUS);

	if (result < 0) 
	{
		LOG_ERR("failed to load atrac3plus module", 0);
		return result;
	}

	LOG_INF("loading PSP mixer modules [done]", 0);

	return 0;
}

int mix_unloadModule()
{
	LOG_INF("unloading PSP mixer modules", 0);

	int result = 0;

	if (sceUtilityUnloadModule(SCE_UTILITY_MODULE_AV_LIBATRAC3PLUS) < 0)
	{
		LOG_ERR("failed to unload atrac3plus module", 0);
		result = -1;
	}

	if (sceUtilityUnloadModule(SCE_UTILITY_MODULE_AV_SASCORE) < 0)
	{
		LOG_ERR("failed to unload sascore module", 0);
		result = -1;
	}

	if (sceUtilityUnloadModule(SCE_UTILITY_MODULE_AV_AVCODEC) < 0)
	{
		LOG_ERR("failed to unload avcodec module", 0);
		result = -1;
	}

	LOG_INF("unloading PSP mixer modules [done]", 0);

	return result;
}

int mix_init()
{
	LOG_INF("initializing PSP mixer", 0);

	int result = 0;

	// initialize PCM output

	result = sceAudioOutput2Reserve(OUTPUT_SIZE);

	if (result < 0)
	{
		LOG_ERR("failed to create audio output", 0);
		return result;
	}

	// initialize media engine

	result = sceSasInitWithGrain(OUTPUT_SIZE);

	if (result < 0)
	{
		LOG_ERR("failed to initialize SAS", 0);
		return result;
	}

	result = sceSasSetOutputmode(SCE_SAS_OUTPUTMODE_STEREO);

	if (result < 0)
	{
		LOG_ERR("failed to set SAS output mode to stereo", 0);
		return result;
	}

	return 0;
}

int mix_shutdown()
{
	LOG_INF("shutting down PSP mixer", 0);

	int result = 0;

	// exit SAS

	if (sceSasExit() < 0)
	{
		LOG_ERR("failed to exit SAS", 0);
		result = -1;
	}

	// flush PCM data

	if (sceAudioOutput2OutputBlocking(0, 0) < 0)
	{
		LOG_ERR("failed to flush PCM data", 0);
		result = -1;
	}

	// stop PCM output

	if (sceAudioOutput2Release() < 0)
	{
		LOG_ERR("failed to end PCM output", 0);
		result = -1;
	}

	return result;
}
