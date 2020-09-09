#include "audiooutput/AudioOutput_Native.h"

#include "framework.h"
#include "framework-allegro2.h"

#include <atomic>
#include <stdio.h>
#include <string.h>

#define DIGI_SAMPLERATE 44100

#define FIXBITS AllegroVoiceApi::FIXBITS
#define INTERP_LINEAR 1

#if defined(DEBUG)
	#define ENABLE_VALIDATION 0
#else
	#define ENABLE_VALIDATION 0
#endif

//

static AllegroTimerApi * s_timerApi = nullptr;

static thread_local AllegroVoiceApi * voiceApi = nullptr;

extern "C"
{
static AudioOutput * audioOutput = nullptr;
static AudioStream_AllegroVoiceMixer * audioStream = nullptr;

int install_timer()
{
	if (s_timerApi != nullptr)
		return -1;
	
	s_timerApi = new AllegroTimerApi(AllegroTimerApi::kMode_Threaded);
	
	return 0;
}

int install_sound(int digi, int midi, const char * cfg_path)
{
	if (voiceApi != nullptr)
		return -1;
	
	voiceApi = new AllegroVoiceApi(DIGI_SAMPLERATE, true);
	
	AudioOutput_Native * audioOutput_native = new AudioOutput_Native();
	audioOutput_native->Initialize(2, DIGI_SAMPLERATE, 64);
	audioOutput = audioOutput_native;
	
	audioStream = new AudioStream_AllegroVoiceMixer(voiceApi, s_timerApi);
	audioOutput->Play(audioStream);
	
	return 0;
}

void allegro_message(const char * format, ...)
{
}

int exists(const char * filename)
{
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
		return 0;
	else
	{
		fclose(file);
		file = nullptr;
		
		return 1;
	}
}

long file_size(const char * filename)
{
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
		return 0;
	else
	{
		fseek(file, 0, SEEK_END);
		
		auto len = ftell(file);
		
		fclose(file);
		file = nullptr;
		
		return len;
	}
}

char * get_extension(const char * filename)
{
	auto len = strlen(filename);
	int dot = -1;
	
	for (int i = 0; i < len; ++i)
		if (filename[i] == '.')
			dot = i;
	
	if (dot == -1)
		return (char*)filename + len;
	else
		return (char*)filename + dot + 1;
}

//

void install_int_ex(void (*proc)(), int speed)
{
	s_timerApi->install_int_ex(proc, speed);
}

void install_int_ex2(void (*proc)(void * data), int speed, void * data)
{
	s_timerApi->install_int_ex2(proc, speed, data);
}

void remove_int(void (*proc)())
{
	s_timerApi->remove_int(proc);
}

void remove_int2(void (*proc)(void * data), void * data)
{
	s_timerApi->remove_int2(proc, data);
}

void set_volume(int, int)
{
}

int allocate_voice(SAMPLE * sample)
{
	return voiceApi->allocate_voice(sample);
}

void reallocate_voice(int voice, SAMPLE * sample)
{
	voiceApi->reallocate_voice(voice, sample);
}

void deallocate_voice(int voice)
{
	voiceApi->deallocate_voice(voice);
}

void voice_start(int voice)
{
	voiceApi->voice_start(voice);
}

void voice_stop(int voice)
{
	voiceApi->voice_stop(voice);
}

int voice_get_volume(int voice)
{
	return voiceApi->voice_get_volume(voice);
}

int voice_get_position(int voice)
{
	return voiceApi->voice_get_position(voice);
}

int voice_get_frequency(int voice)
{
	return voiceApi->voice_get_frequency(voice);
}

int voice_get_pan(int voice)
{
	return voiceApi->voice_get_pan(voice);
}

void voice_set_volume(int voice, int volume)
{
	voiceApi->voice_set_volume(voice, volume);
}

void voice_set_playmode(int voice, int mode)
{
	voiceApi->voice_set_playmode(voice, mode);
}

void voice_set_position(int voice, int position)
{
	voiceApi->voice_set_position(voice, position);
}

void voice_set_frequency(int voice, int freq)
{
	voiceApi->voice_set_frequency(voice, freq);
}

void voice_set_pan(int voice, int pan)
{
	voiceApi->voice_set_pan(voice, pan);
}

void lock_sample(SAMPLE * sample)
{
}

}

AudioStream_AllegroVoiceMixer::AudioStream_AllegroVoiceMixer(AllegroVoiceApi * in_voiceApi, AllegroTimerApi * in_timerApi)
	: AudioStream()
	, voiceApi(in_voiceApi)
	, timerApi(in_timerApi)
{
}

int AudioStream_AllegroVoiceMixer::Provide(int numSamples, AudioSample* __restrict buffer)
{
	if (timerApi != nullptr && timerApi->mode == AllegroTimerApi::kMode_Manual)
	{
		::voiceApi = this->voiceApi;
		timerApi->processInterrupts(int64_t(numSamples) * 1000000 / voiceApi->sampleRate);
		::voiceApi = nullptr;
	}
	
	int * __restrict mixingBuffer = (int*)alloca(numSamples * 2 * sizeof(int));
	memset(mixingBuffer, 0, numSamples * 2 * sizeof(int));
	
	voiceApi->lock();
	{
	#if 0
		float * __restrict samplesL = (float*)alloca(numSamples * sizeof(float));
		float * __restrict samplesR = (float*)alloca(numSamples * sizeof(float));
		
		memset(samplesL, 0, numSamples * sizeof(float));
		memset(samplesR, 0, numSamples * sizeof(float));
		
		float * __restrict samplesV = (float*)alloca(numSamples * sizeof(float));
		
		for (int i = 0; i < AllegroVoiceApi::MAX_VOICES; ++i)
		{
			float stereoPanning;
			
			if (voiceApi->generateSamplesForVoice(i, samplesV, numSamples, stereoPanning))
			{
				const float panL = 1.f - stereoPanning;
				const float panR =       stereoPanning;
				
				for (int j = 0; j < numSamples; ++j)
				{
					samplesL[j] += samplesV[j] * panL;
					samplesR[j] += samplesV[j] * panR;
				}
			}
		}
		
		for (int i = 0; i < numSamples; ++i)
		{
			mixingBuffer[i * 2 + 0] = samplesL[i] * (1 << (15 - 2));
			mixingBuffer[i * 2 + 1] = samplesR[i] * (1 << (15 - 2));
		}
	#else
		for (auto & __restrict voice : voiceApi->voices)
		{
			if (voice.used && voice.started && voice.sample->len != 0)
			{
				const int pan = voice.pan;
				
				const int pan1 = ((1 << 8) - pan) * voice.volume;
				const int pan2 = (           pan) * voice.volume;
				
				int sampleIndex = voice.position >> FIXBITS;
				
				for (int i = 0; i < numSamples; ++i)
				{
				#if ENABLE_VALIDATION
					if (voice.playmode & PLAYMODE_LOOP)
						Assert(sampleIndex >= 0 && sampleIndex < voice.sample->loop_end);
					else
						Assert(sampleIndex >= 0 && sampleIndex < voice.sample->len);
				#endif
				
					if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
					{
					#if INTERP_LINEAR
						const int sampleIndex2 =
							(voice.playmode & PLAYMODE_LOOP) != 0
							? (sampleIndex + 1 < voice.sample->loop_end ? sampleIndex + 1 : sampleIndex)
							: (sampleIndex + 1 < voice.sample->len      ? sampleIndex + 1 : sampleIndex);
						
						const int t = (voice.position >> (FIXBITS - 16)) & 0xffff;
					#endif
					
						if (voice.sample->bits == 8)
						{
							const unsigned char * __restrict values = (unsigned char*)voice.sample->data;
							
						#if INTERP_LINEAR
							const int value1 = int8_t(values[sampleIndex ] ^ 0x80);
							const int value2 = int8_t(values[sampleIndex2] ^ 0x80);
							
							const int value = (value1 * ((1 << 16) - t) + value2 * t) >> 16;
							
						#if ENABLE_VALIDATION
							if (value1 < value2)
								Assert(value >= value1 && value <= value2);
							else if (value1 > value2)
								Assert(value <= value1 && value >= value2);
							else
								Assert(value == value1 && value == value2);
						#endif
						#else
							const int value = int8_t(values[sampleIndex] ^ 0x80);
						#endif
							
							mixingBuffer[i * 2 + 0] += (value * pan1) >> 8;
							mixingBuffer[i * 2 + 1] += (value * pan2) >> 8;
						}
						else if (voice.sample->bits == 16)
						{
							const unsigned short * __restrict values = (unsigned short*)voice.sample->data;
							
						#if INTERP_LINEAR
							const int value1 = int16_t(values[sampleIndex ] ^ 0x8000);
							const int value2 = int16_t(values[sampleIndex2] ^ 0x8000);
							
							const int value = (value1 * ((1 << 16) - t) + value2 * t) >> 16;
							
						#if ENABLE_VALIDATION
							if (value1 < value2)
								Assert(value >= value1 && value < value2);
							else if (value2 < value1)
								Assert(value <= value1 && value >= value2);
							else
								Assert(value == value1 && value == value2);
						#endif
						#else
							const int value = int16_t(values[sampleIndex] ^ 0x8000);
						#endif
							
							mixingBuffer[i * 2 + 0] += (value * pan1) >> 16;
							mixingBuffer[i * 2 + 1] += (value * pan2) >> 16;
						}
					}
					
					// increment sample playback position
					
					voice.position += voice.sampleIncrement;
					
					sampleIndex = voice.position >> FIXBITS;
					
					// handle looping and ping-pong
					
					if (voice.playmode & PLAYMODE_LOOP)
					{
						if (voice.playmode & PLAYMODE_BIDIR)
						{
							if (voice.sampleIncrement > 0)
							{
								if (sampleIndex >= voice.sample->loop_end)
								{
								#if 1
									voice.position = (int64_t(voice.sample->loop_end - 1) << FIXBITS << 1) - voice.position;
									Assert(voice.position >= 0);
									
									sampleIndex = voice.position >> FIXBITS;
								#elif 1
									const int64_t delta = voice.position - (int64_t(voice.sample->loop_end) << FIXBITS);
									
									voice.position -= delta * 2;
									
									sampleIndex = voice.position >> FIXBITS;
								#else
									voice.position = int64_t(voice.sample->loop_end) << FIXBITS;
									
									sampleIndex = voice.sample->loop_end;
								#endif
								
									voice.sampleIncrement = -voice.sampleIncrement;
									
								#if ENABLE_VALIDATION
									const int64_t min = int64_t(voice.sample->loop_start) << FIXBITS;
									const int64_t max = int64_t(voice.sample->loop_end) << FIXBITS;
									Assert(voice.position >= min && voice.position < max);
									Assert(sampleIndex >= voice.sample->loop_start);
									Assert(sampleIndex < voice.sample->loop_end);
								#endif
								}
							}
							else
							{
								if (sampleIndex < voice.sample->loop_start)
								{
								#if 1
									voice.position = (int64_t(voice.sample->loop_start) << FIXBITS << 1) - voice.position;
									Assert(voice.position >= 0);
									
									sampleIndex = voice.position >> FIXBITS;
								#elif 1
									const int64_t delta = voice.position - (int64_t(voice.sample->loop_start) << FIXBITS);
									
									voice.position -= delta * 2;
									
									sampleIndex = voice.position >> FIXBITS;
								#else
									voice.position = int64_t(voice.sample->loop_start) << FIXBITS;
									
									sampleIndex = voice.sample->loop_start;
								#endif
								
									voice.sampleIncrement = -voice.sampleIncrement;
									
								#if ENABLE_VALIDATION
									const int64_t min = int64_t(voice.sample->loop_start) << FIXBITS;
									const int64_t max = int64_t(voice.sample->loop_end) << FIXBITS;
									Assert(voice.position >= min && voice.position < max);
									Assert(sampleIndex >= voice.sample->loop_start);
									Assert(sampleIndex < voice.sample->loop_end);
								#endif
								}
							}
						}
						else
						{
							if (sampleIndex >= voice.sample->loop_end)
							{
							#if 1
								voice.position -= int64_t(voice.sample->loop_end - voice.sample->loop_start) << FIXBITS;
								
								sampleIndex = voice.position >> FIXBITS;
							#elif 1
								const int64_t delta = voice.position - (int64_t(voice.sample->loop_end) << FIXBITS);
								
								voice.position = (int64_t(voice.sample->loop_start) << FIXBITS) + delta;
								
								sampleIndex = voice.position >> FIXBITS;
							#else
								sampleIndex = voice.sample->loop_start;
								
								voice.position = int64_t(voice.sample->loop_start) << FIXBITS;
							#endif
							
							#if ENABLE_VALIDATION
								const int64_t min = int64_t(voice.sample->loop_start) << FIXBITS;
								const int64_t max = int64_t(voice.sample->loop_end) << FIXBITS;
								Assert(voice.position >= min && voice.position < max);
								Assert(sampleIndex >= voice.sample->loop_start);
								Assert(sampleIndex < voice.sample->loop_end);
							#endif
							}
						}
					}
					else
					{
						if (sampleIndex >= voice.sample->len)
						{
							//Assert(sampleIndex == voice.sample->len); // not necessarily true, as sample playback may increment by more than one sample
							
							voice.started = false;
							
							break;
						}
					}
				}
			}
		}
	#endif
	}
	voiceApi->unlock();
	
	for (int i = 0; i < numSamples; ++i)
	{
		int vL = mixingBuffer[i * 2 + 0];
		int vR = mixingBuffer[i * 2 + 1];
	
		if (vL > INT16_MAX)
			vL = INT16_MAX;
		if (vL < INT16_MIN)
			vL = INT16_MIN;

		if (vR > INT16_MAX)
			vR = INT16_MAX;
		if (vR < INT16_MIN)
			vR = INT16_MIN;

		buffer[i].channel[0] = vL;
		buffer[i].channel[1] = vR;
	}
	
	return numSamples;
}
