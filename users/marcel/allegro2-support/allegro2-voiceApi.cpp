#include "allegro2-voiceApi.h"
#include <assert.h>
#include <SDL2/SDL.h>
#include <string.h>

#define FIXBITS AllegroVoiceApi::FIXBITS
#define INTERP_LINEAR 1

#define Assert assert
#define Verify(x) do { const bool y = x; (void)y; Assert(y); } while (false)

AllegroVoiceApi::AllegroVoiceApi(const int in_sampleRate, const bool in_useMutex)
	: sampleRate(in_sampleRate)
	, mutex(nullptr)
	, useMutex(in_useMutex)
{
	if (useMutex)
	{
		mutex = SDL_CreateMutex();
	}
}

AllegroVoiceApi::~AllegroVoiceApi()
{
	if (useMutex)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
}

int AllegroVoiceApi::allocate_voice(SAMPLE * sample)
{
	int result = -1;
	
	lock();
	{
		for (int i = 0; i < MAX_VOICES; ++i)
		{
			if (voices[i].used == 0)
			{
				voices[i] = VoiceInfo();
				
				voices[i].used = 1;
				voices[i].sample = sample;
				voices[i].frequency = sample->freq;
				
				voices[i].updateIncrement(+1, sampleRate);
				
				result = i;
				
				break;
			}
		}
	}
	unlock();
	
	return result;
}

void AllegroVoiceApi::reallocate_voice(int voice, SAMPLE * sample)
{
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice] = VoiceInfo();
		
		voices[voice].used = 1;
		voices[voice].sample = sample;
		voices[voice].frequency = sample->freq;
		
		voices[voice].updateIncrement(+1, sampleRate);
	}
	unlock();
}

void AllegroVoiceApi::deallocate_voice(int voice)
{
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].used = 0;
	}
	unlock();
}

void AllegroVoiceApi::voice_start(int voice)
{
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		const int sampleIndex = voices[voice].position >> FIXBITS;
		
		if (sampleIndex >= voices[voice].sample->len)
			voices[voice].position = 0;
		
		voices[voice].started = 1;
	}
	unlock();
}

void AllegroVoiceApi::voice_stop(int voice)
{
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].started = 0;
	}
	unlock();
}

int AllegroVoiceApi::voice_get_position(int voice)
{
	if (voice == -1)
		return 0;
	
	int result;
	
	lock();
	{
		Assert(voices[voice].used);
		
		const int sampleIndex = voices[voice].position >> FIXBITS;
		
		// return -1 if sample playback has ended
		
		if (sampleIndex >= voices[voice].sample->len)
			result = -1;
		else
		{
			Assert(sampleIndex >= 0 && sampleIndex < voices[voice].sample->len);
			
			result = sampleIndex;
		}
	}
	unlock();
	
	return result;
}

int AllegroVoiceApi::voice_get_frequency(int voice)
{
	if (voice == -1)
		return 0;
	
	Assert(voices[voice].used);
	
	return voices[voice].frequency;
}

void AllegroVoiceApi::voice_set_volume(int voice, int volume)
{
	Assert(volume >= 0 && volume <= 256);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].volume = volume;
	}
	unlock();
}

void AllegroVoiceApi::voice_set_playmode(int voice, int mode)
{
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].playmode = mode;
	}
	unlock();
}

void AllegroVoiceApi::voice_set_position(int voice, int position)
{
	Assert(position >= 0);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		if (position < 0)
			position = 0;
		
		voices[voice].position = int64_t(position) << FIXBITS;
		
		if (position >= voices[voice].sample->len)
			voices[voice].started = 0;
	}
	unlock();
}

void AllegroVoiceApi::voice_set_frequency(int voice, int freq)
{
	Assert(freq >= 0);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].frequency = freq;
		
		voices[voice].updateIncrement(voices[voice].sampleIncrement < 0 ? -1 : +1, sampleRate);
	}
	unlock();
}

void AllegroVoiceApi::voice_set_pan(int voice, int pan)
{
	Assert(pan >= 0 && pan <= 256);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].pan = pan;
	}
	unlock();
}

void AllegroVoiceApi::lock()
{
	if (useMutex)
	{
		Verify(SDL_LockMutex(mutex) == 0);
	}
}

void AllegroVoiceApi::unlock()
{
	if (useMutex)
	{
		Verify(SDL_UnlockMutex(mutex) == 0);
	}
}

bool AllegroVoiceApi::generateSamplesForVoice(const int voiceIndex, float * __restrict samples, const int numSamples, float & stereoPanning)
{
	auto & voice = voices[voiceIndex];
	
	if (voiceIndex < 0 || voiceIndex >= MAX_VOICES || !(voice.used && voice.started && voice.sample->len != 0))
	{
		memset(samples, 0, numSamples * sizeof(float));
		
		stereoPanning = .5f;
		
		return false;
	}
	
	stereoPanning = voice.pan / 256.f;
	
	int64_t voice_position = voice.position;
	
	int sampleIndex = voice_position >> FIXBITS;
	
	const float scale16 = 1.f / float(1 << (16 - 1));
	const float scale24 = 1.f / float(1 << (24 - 1));
	
	for (int i = 0; i < numSamples; ++i)
	{
		Assert(sampleIndex >= 0 && sampleIndex < voice.sample->len);
		
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
			#else
				const int value = int8_t(values[sampleIndex] ^ 0x80);
			#endif
				
				samples[i] = (value * voice.volume) * scale16;
			}
			else if (voice.sample->bits == 16)
			{
				const unsigned short * __restrict values = (unsigned short*)voice.sample->data;
				
			#if INTERP_LINEAR
				const int value1 = int16_t(values[sampleIndex ] ^ 0x8000);
				const int value2 = int16_t(values[sampleIndex2] ^ 0x8000);
	
				const int value = (value1 * ((1 << 16) - t) + value2 * t) >> 16;
			#else
				const int value = int16_t(values[sampleIndex] ^ 0x8000);
			#endif
				
				samples[i] = (value * voice.volume) * scale24;
			}
		}
		else
		{
			samples[i] = 0.f;
		}
		
		// increment sample playback position
		
		voice_position += voice.sampleIncrement;
		
		sampleIndex = voice_position >> FIXBITS;
		
		// handle looping and ping-pong
		
		if (voice.playmode & PLAYMODE_LOOP)
		{
			if (voice.playmode & PLAYMODE_BIDIR)
			{
				if (voice.sampleIncrement > 0)
				{
					if (sampleIndex >= voice.sample->loop_end)
					{
						voice_position = (int64_t(voice.sample->loop_end - 1) << FIXBITS << 1) - voice_position;
						
						sampleIndex = voice_position >> FIXBITS;
						
						voice.sampleIncrement = -voice.sampleIncrement;
					}
				}
				else
				{
					if (sampleIndex < voice.sample->loop_start)
					{
						voice_position = (int64_t(voice.sample->loop_start) << FIXBITS << 1) - voice_position;
						
						sampleIndex = voice_position >> FIXBITS;
						
						voice.sampleIncrement = -voice.sampleIncrement;
					}
				}
			}
			else
			{
				if (sampleIndex >= voice.sample->loop_end)
				{
					voice_position -= int64_t(voice.sample->loop_end - voice.sample->loop_start) << FIXBITS;
					
					sampleIndex = voice_position >> FIXBITS;
				}
			}
		}
		else
		{
			if (sampleIndex >= voice.sample->len)
			{
				//Assert(sampleIndex == voice.sample->len); // not necessarily true, as sample playback may increment by more than one sample
				
				voice.started = false;
				
				for (int j = i + 1; j < numSamples; ++j)
					samples[j] = 0.f;
				
				break;
			}
		}
	}
	
	voice.position = voice_position;
	
	return true;
}
