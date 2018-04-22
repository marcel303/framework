#include "audiostream/AudioOutput_PortAudio.h"
#include "framework.h"
#include "framework-allegro2.h"
#include <atomic>
#include <stdio.h>
#include <string.h>

//#define DIGI_SAMPLERATE 44100
#define DIGI_SAMPLERATE 192000

#define PROCESS_INTERRUPTS_ON_AUDIO_THREAD 1

//

int AllegroVoiceAPI::allocate_voice(SAMPLE * sample)
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

void AllegroVoiceAPI::reallocate_voice(int voice, SAMPLE * sample)
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

void AllegroVoiceAPI::deallocate_voice(int voice)
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

void AllegroVoiceAPI::voice_start(int voice)
{
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].started = 1;
	}
	unlock();
}

void AllegroVoiceAPI::voice_stop(int voice)
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

int AllegroVoiceAPI::voice_get_position(int voice)
{
	if (voice == -1)
		return 0;
	
	Assert(voices[voice].used);
	
	return voices[voice].position;
}

int AllegroVoiceAPI::voice_get_frequency(int voice)
{
	if (voice == -1)
		return 0;
	
	Assert(voices[voice].used);
	
	return voices[voice].frequency;
}

void AllegroVoiceAPI::voice_set_volume(int voice, int volume)
{
	Assert(volume >= 0 && volume <= 255);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].volume = volume;
	}
	unlock();
}

void AllegroVoiceAPI::voice_set_playmode(int voice, int mode)
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

void AllegroVoiceAPI::voice_set_position(int voice, int position)
{
	Assert(position >= 0);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].position = int64_t(position) << 16;
	}
	unlock();
}

void AllegroVoiceAPI::voice_set_frequency(int voice, int freq)
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

void AllegroVoiceAPI::voice_set_pan(int voice, int pan)
{
	Assert(pan >= 0 && pan <= 255);
	
	if (voice == -1)
		return;
	
	lock();
	{
		Assert(voices[voice].used);
		
		voices[voice].pan = pan;
	}
	unlock();
}

void AllegroVoiceAPI::generateSamplesForVoice(const int voiceIndex, float * __restrict samples, const int numSamples, float & stereoPanning)
{
	auto & voice = voices[voiceIndex];
	
	if (voiceIndex < 0 || voiceIndex >= MAX_VOICES || !(voice.used && voice.started && voice.sample->len != 0))
	{
		memset(samples, 0, numSamples * sizeof(float));
		
		stereoPanning = .5f;
	}
	
	stereoPanning = voice.pan / 255.f;
	
	int sampleIndex = voice.position >> 16;
	
	for (int i = 0; i < numSamples; ++i)
	{
		if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
		{
			if (voice.sample->bits == 8)
			{
				const unsigned char * values = (unsigned char*)voice.sample->data;
				
				const int value = int8_t(values[sampleIndex] ^ 0x80) * voice.volume;
				
				samples[i] = value / float(1 << 15);
			}
			else if (voice.sample->bits == 16)
			{
				const unsigned short * values = (unsigned short*)voice.sample->data;
				
				const int value = int16_t(values[sampleIndex] ^ 0x8000) * voice.volume;
				
				samples[i] = value / float(1 << 23);
			}
		}
		
		// increment sample playback position
		
		voice.position += voice.sampleIncrement;
		
		sampleIndex = voice.position >> 16;
		
		// handle looping and ping-pong
		
		if (voice.playmode & PLAYMODE_LOOP)
		{
			if (voice.playmode & PLAYMODE_BIDIR)
			{
				if (voice.sampleIncrement > 0)
				{
					if (sampleIndex >= voice.sample->loop_end)
					{
						voice.position = (int64_t(voice.sample->loop_end - 1) << 16 << 1) - voice.position;
						
						sampleIndex = voice.position >> 16;
						
						voice.sampleIncrement = -voice.sampleIncrement;
					}
				}
				else
				{
					if (sampleIndex < voice.sample->loop_start)
					{
						voice.position = (int64_t(voice.sample->loop_start) << 16 << 1) - voice.position;
						
						sampleIndex = voice.position >> 16;
						
						voice.sampleIncrement = -voice.sampleIncrement;
					}
				}
			}
			else
			{
				if (sampleIndex >= voice.sample->loop_end)
				{
					voice.position -= int64_t(voice.sample->loop_end - voice.sample->loop_start) << 16;
					
					sampleIndex = voice.position >> 16;
				}
			}
		}
	}
}

static thread_local AllegroVoiceAPI * voiceAPI = nullptr;

extern "C"
{
static AudioOutput_PortAudio * audioOutput = nullptr;
static AudioStream_AllegroVoiceMixer * audioStream = nullptr;

int install_sound(int digi, int midi, const char * cfg_path)
{
	voiceAPI = new AllegroVoiceAPI(DIGI_SAMPLERATE);
	
	audioOutput = new AudioOutput_PortAudio();
	audioOutput->Initialize(2, DIGI_SAMPLERATE, 64);
	
	audioStream = new AudioStream_AllegroVoiceMixer(voiceAPI);
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

struct TimerReg
{
	TimerReg * next;
	
	void (*proc)();
	std::atomic<bool> stop;
	int delay;
	int delayInSamples;
	
#if PROCESS_INTERRUPTS_ON_AUDIO_THREAD
	int sampleTime;
#else
	SDL_Thread * thread;
#endif
};

static TimerReg * s_timerRegs = nullptr;

#if !PROCESS_INTERRUPTS_ON_AUDIO_THREAD

#include <unistd.h>

static int TimerThreadProc(void * obj)
{
	TimerReg * r = (TimerReg*)obj;
	
	// todo : use POSIX timer API ?
	
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
	
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	
	while (r->stop == false)
	{
		audioStream->lock();
		{
			r->proc();
		}
		audioStream->unlock();
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		
		const int64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
		
		if (delta_us < r->delay)
		{
			const int64_t todo_us = r->delay - delta_us;
			
			usleep(todo_us);
		}
		else
		{
			printf("no wait..\n");
		}
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	}
	
	return 0;
}

#endif

void install_int_ex(void (*proc)(), int speed)
{
	for (auto r = s_timerRegs; r != nullptr; r = s_timerRegs->next)
	{
		if (r->proc == proc)
		{
			r->delay = speed;
			r->delayInSamples = (int64_t(speed) * DIGI_SAMPLERATE) / 1000000;
			return;
		}
	}
	
	TimerReg * r = new TimerReg;
	
	r->proc = proc;
	r->stop = false;
	r->delay = speed;
	r->delayInSamples = (int64_t(speed) * DIGI_SAMPLERATE) / 1000000;
	
#if PROCESS_INTERRUPTS_ON_AUDIO_THREAD
	r->sampleTime = 0;
	
	audioStream->lock();
	{
		r->next = s_timerRegs;
		s_timerRegs = r;
	}
	audioStream->unlock();
#else
	r->next = s_timerRegs;
	s_timerRegs = r;
	
	r->thread = SDL_CreateThread(TimerThreadProc, "Allegro timer", r);
#endif
}

void remove_int(void (*proc)())
{
	TimerReg ** r = &s_timerRegs;
	
	while ((*r) != nullptr)
	{
		TimerReg * t = *r;
		
		if ((*r)->proc == proc)
		{
		#if PROCESS_INTERRUPTS_ON_AUDIO_THREAD
			audioStream->lock();
		#endif
		
			*r = t->next;
			
		#if PROCESS_INTERRUPTS_ON_AUDIO_THREAD
			audioStream->unlock();
		#endif
		
			//
			
		#if !PROCESS_INTERRUPTS_ON_AUDIO_THREAD
			t->stop = true;
			
			SDL_WaitThread(t->thread, nullptr);
		#endif
			
			delete t;
			t = nullptr;
		}
		else
		{
			r = &t->next;
		}
	}
}

#if PROCESS_INTERRUPTS_ON_AUDIO_THREAD

static void processInterrupts(const int numSamples)
{
	for (TimerReg * r = s_timerRegs; r != nullptr; r = r->next)
	{
		r->sampleTime += numSamples;
		
		if (r->sampleTime >= r->delayInSamples)
		{
			r->sampleTime -= r->delayInSamples;
			
			r->proc();
		}
	}
}

#endif

void set_volume(int, int)
{
}

int allocate_voice(SAMPLE * sample)
{
	return voiceAPI->allocate_voice(sample);
}

void reallocate_voice(int voice, SAMPLE * sample)
{
	voiceAPI->reallocate_voice(voice, sample);
}

void deallocate_voice(int voice)
{
	voiceAPI->deallocate_voice(voice);
}

void voice_start(int voice)
{
	voiceAPI->voice_start(voice);
}

void voice_stop(int voice)
{
	voiceAPI->voice_stop(voice);
}

int voice_get_position(int voice)
{
	return voiceAPI->voice_get_position(voice);
}

int voice_get_frequency(int voice)
{
	return voiceAPI->voice_get_frequency(voice);
}

void voice_set_volume(int voice, int volume)
{
	voiceAPI->voice_set_volume(voice, volume);
}

void voice_set_playmode(int voice, int mode)
{
	voiceAPI->voice_set_playmode(voice, mode);
}

void voice_set_position(int voice, int position)
{
	voiceAPI->voice_set_position(voice, position);
}

void voice_set_frequency(int voice, int freq)
{
	voiceAPI->voice_set_frequency(voice, freq);
}

void voice_set_pan(int voice, int pan)
{
	voiceAPI->voice_set_pan(voice, pan);
}

void lock_sample(SAMPLE * sample)
{
}

}

AudioStream_AllegroVoiceMixer::AudioStream_AllegroVoiceMixer(AllegroVoiceAPI * _voiceAPI)
	: AudioStream()
	, mutex(nullptr)
	, voiceAPI(_voiceAPI)
{
	mutex = SDL_CreateMutex();
}

AudioStream_AllegroVoiceMixer::~AudioStream_AllegroVoiceMixer()
{
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

void AudioStream_AllegroVoiceMixer::lock()
{
	SDL_LockMutex(mutex);
}

void AudioStream_AllegroVoiceMixer::unlock()
{
	SDL_UnlockMutex(mutex);
}

int AudioStream_AllegroVoiceMixer::Provide(int numSamples, AudioSample* __restrict buffer)
{
#if PROCESS_INTERRUPTS_ON_AUDIO_THREAD
	::voiceAPI = this->voiceAPI;
	processInterrupts(numSamples);
	::voiceAPI = nullptr;
#endif
	
	memset(buffer, 0, numSamples * sizeof(AudioSample));
	
	lock();
	{
		for (auto & __restrict voice : voiceAPI->voices)
		{
			if (voice.used && voice.started && voice.sample->len != 0)
			{
				const int pan1 = 0xff - voice.pan;
				const int pan2 =        voice.pan;
				
				int sampleIndex = voice.position >> 16;
				
				for (int i = 0; i < numSamples; ++i)
				{
					Assert(sampleIndex >= 0 && sampleIndex < voice.sample->len);
					
					if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
					{
						if (voice.sample->bits == 8)
						{
							const unsigned char * __restrict values = (unsigned char*)voice.sample->data;
							
							const int value = int8_t(values[sampleIndex] ^ 0x80) * voice.volume;
							
							buffer[i].channel[0] += (value * pan1) >> 8 >> 2;
							buffer[i].channel[1] += (value * pan2) >> 8 >> 2;
						}
						else if (voice.sample->bits == 16)
						{
							const unsigned short * __restrict values = (unsigned short*)voice.sample->data;
							
							const int value = int16_t(values[sampleIndex] ^ 0x8000);
							
							buffer[i].channel[0] += (((value * voice.volume) >> 1) * pan1) >> 15 >> 2;
							buffer[i].channel[1] += (((value * voice.volume) >> 1) * pan2) >> 15 >> 2;
						}
					}
					
					// increment sample playback position
					
					voice.position += voice.sampleIncrement;
					
					sampleIndex = voice.position >> 16;
					
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
									voice.position = (int64_t(voice.sample->loop_end - 1) << 16 << 1) - voice.position;
									
									sampleIndex = voice.position >> 16;
								#elif 1
									const int64_t delta = voice.position - (int64_t(voice.sample->loop_end) << 16);
									
									voice.position -= delta * 2;
									
									sampleIndex = voice.position >> 16;
								#else
									voice.position = int64_t(voice.sample->loop_end) << 16;
									
									sampleIndex = voice.sample->loop_end;
								#endif
								
									voice.sampleIncrement = -voice.sampleIncrement;
								}
							}
							else
							{
								if (sampleIndex < voice.sample->loop_start)
								{
								#if 1
									voice.position = (int64_t(voice.sample->loop_start) << 16 << 1) - voice.position;
									
									sampleIndex = voice.position >> 16;
								#elif 1
									const int64_t delta = voice.position - (int64_t(voice.sample->loop_start) << 16);
									
									voice.position -= delta * 2;
									
									sampleIndex = voice.position >> 16;
								#else
									voice.position = int64_t(voice.sample->loop_start) << 16;
									
									sampleIndex = voice.sample->loop_start;
								#endif
								
									voice.sampleIncrement = -voice.sampleIncrement;
								}
							}
						}
						else
						{
							if (sampleIndex >= voice.sample->loop_end)
							{
							#if 1
								voice.position -= int64_t(voice.sample->loop_end - voice.sample->loop_start) << 16;
								
								sampleIndex = voice.position >> 16;
							#elif 1
								const int64_t delta = voice.position - (int64_t(voice.sample->loop_end) << 16);
								
								voice.position = (voice.sample->loop_start << 16) + delta;
								
								sampleIndex = voice.position >> 16;
							#else
								sampleIndex = voice.sample->loop_start;
								
								voice.position = int64_t(voice.sample->loop_start) << 16;
							#endif
							}
						}
					}
					else
					{
						if (sampleIndex >= voice.sample->len)
						{
							voice.started = false;
							
							break;
						}
					}
				}
			}
		}
	}
	unlock();
	
	return numSamples;
}
