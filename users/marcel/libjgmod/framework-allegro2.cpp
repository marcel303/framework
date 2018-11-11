#include "audiostream/AudioOutput_PortAudio.h"
#include "framework.h"

#include "framework-allegro2.h"

#include <atomic>
#include <stdio.h>
#include <string.h>

//#define DIGI_SAMPLERATE 44100
#define DIGI_SAMPLERATE 192000

#define INTERP_LINEAR 1

#define FIXBITS 32

//

static int TimerThreadProc(void * obj);

//

typedef void (*TimerProc)();

struct AllegroTimerReg
{
	AllegroTimerReg * next;
	AllegroTimerApi * owner;
	
	void (*proc)(void * data);
	void * data;
	std::atomic<bool> stop;
	int delay;
	int64_t delayInMilliSamples;
	
	int sampleTime; // when processing manually
	SDL_Thread * thread; // when using threaded processing
};

//


AllegroTimerApi::AllegroTimerApi(const Mode in_mode)
	: mutex(nullptr)
	, mode(in_mode)
{
	mutex = SDL_CreateMutex();
}

AllegroTimerApi::~AllegroTimerApi()
{
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

void AllegroTimerApi::handle_int(void * data)
{
	TimerProc proc = (TimerProc)data;
	
	proc();
}

void AllegroTimerApi::install_int_ex(void (*proc)(), int speed)
{
	install_int_ex2(handle_int, speed, (void*)proc);
}

void AllegroTimerApi::install_int_ex2(void (*proc)(void * data), int speed, void * data)
{
	bool preexisting = false;
	
	lock();
	{
		for (auto r = timerRegs; r != nullptr; r = timerRegs->next)
		{
			if (r->proc == proc && r->data == data)
			{
				r->delay = speed;
				r->delayInMilliSamples = (int64_t(speed) * DIGI_SAMPLERATE) / 1000;
				preexisting = true;
				break;
			}
		}
	}
	unlock();
	
	if (preexisting)
		return;
	
	AllegroTimerReg * r = new AllegroTimerReg();
	
	r->owner = this;
	r->proc = proc;
	r->data = data;
	r->stop = false;
	r->delay = speed;
	r->delayInMilliSamples = (int64_t(speed) * DIGI_SAMPLERATE) / 1000;
	
	r->sampleTime = 0;
	r->thread = nullptr;
	
	lock();
	{
		r->next = timerRegs;
		timerRegs = r;
	}
	unlock();
	
	if (mode == kMode_Threaded)
	{
		r->thread = SDL_CreateThread(TimerThreadProc, "Allegro timer", r);
	}
}

void AllegroTimerApi::remove_int(void (*proc)())
{
	remove_int2(handle_int, (void*)proc);
}

void AllegroTimerApi::remove_int2(void (*proc)(void * data), void * data)
{
	lock();
	
	AllegroTimerReg ** r = &timerRegs;
	
	while ((*r) != nullptr)
	{
		AllegroTimerReg * t = *r;
		
		if ((*r)->proc == proc && (*r)->data == data)
		{
			*r = t->next;
			
			//
			
			if (t->thread != nullptr)
			{
				unlock(); // temporarily give up our lock, to make sure the work executing on the thread has access to the timer api too
				{
					t->stop = true;
				
					SDL_WaitThread(t->thread, nullptr);
				}
				lock();
			}
			
			delete t;
			t = nullptr;
		}
		else
		{
			r = &t->next;
		}
	}
	
	unlock();
}

void AllegroTimerApi::lock()
{
	Verify(SDL_LockMutex(mutex) == 0);
}

void AllegroTimerApi::unlock()
{
	Verify(SDL_UnlockMutex(mutex) == 0);
}

void AllegroTimerApi::processInterrupts(const int numSamples)
{
	Assert(mode == kMode_Manual);
	if (mode != kMode_Manual)
		return;
	
	lock();
	{
		for (AllegroTimerReg * r = timerRegs; r != nullptr; r = r->next)
		{
			r->sampleTime += numSamples * 1000;
			
			if (r->sampleTime >= r->delayInMilliSamples)
			{
				r->sampleTime -= r->delayInMilliSamples;
				
				r->proc(r->data);
			}
		}
	}
	unlock();
}

//

AllegroVoiceAPI::AllegroVoiceAPI(const int in_sampleRate)
	: sampleRate(in_sampleRate)
	, mutex(nullptr)
{
	mutex = SDL_CreateMutex();
}

AllegroVoiceAPI::~AllegroVoiceAPI()
{
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

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
		
		const int sampleIndex = voices[voice].position >> FIXBITS;
		
		if (sampleIndex >= voices[voice].sample->len)
			voices[voice].position = 0;
		
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
		
		if (position < 0)
			position = 0;
		
		voices[voice].position = int64_t(position) << FIXBITS;
		
		if (position >= voices[voice].sample->len)
			voices[voice].started = 0;
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

void AllegroVoiceAPI::lock()
{
	Verify(SDL_LockMutex(mutex) == 0);
}

void AllegroVoiceAPI::unlock()
{
	Verify(SDL_UnlockMutex(mutex) == 0);
}

bool AllegroVoiceAPI::generateSamplesForVoice(const int voiceIndex, float * __restrict samples, const int numSamples, float & stereoPanning)
{
	auto & voice = voices[voiceIndex];
	
	if (voiceIndex < 0 || voiceIndex >= MAX_VOICES || !(voice.used && voice.started && voice.sample->len != 0))
	{
		memset(samples, 0, numSamples * sizeof(float));
		
		stereoPanning = .5f;
		
		return false;
	}
	
	stereoPanning = voice.pan / 255.f;
	
	int sampleIndex = voice.position >> FIXBITS;
	
	for (int i = 0; i < numSamples; ++i)
	{
		Assert(sampleIndex >= 0 && sampleIndex < voice.sample->len);
		
		if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
		{
			if (voice.sample->bits == 8)
			{
				const unsigned char * values = (unsigned char*)voice.sample->data;
				
				const int value = int8_t(values[sampleIndex] ^ 0x80) * voice.volume;
				
				samples[i] = value / float(1 << (16 - 1));
			}
			else if (voice.sample->bits == 16)
			{
				const unsigned short * values = (unsigned short*)voice.sample->data;
				
				const int value = int16_t(values[sampleIndex] ^ 0x8000) * voice.volume;
				
				samples[i] = value / float(1 << (24 - 1));
			}
		}
		else
		{
			samples[i] = 0.f;
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
						voice.position = (int64_t(voice.sample->loop_end - 1) << FIXBITS << 1) - voice.position;
						
						sampleIndex = voice.position >> FIXBITS;
						
						voice.sampleIncrement = -voice.sampleIncrement;
					}
				}
				else
				{
					if (sampleIndex < voice.sample->loop_start)
					{
						voice.position = (int64_t(voice.sample->loop_start) << FIXBITS << 1) - voice.position;
						
						sampleIndex = voice.position >> FIXBITS;
						
						voice.sampleIncrement = -voice.sampleIncrement;
					}
				}
			}
			else
			{
				if (sampleIndex >= voice.sample->loop_end)
				{
					voice.position -= int64_t(voice.sample->loop_end - voice.sample->loop_start) << FIXBITS;
					
					sampleIndex = voice.position >> FIXBITS;
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
	
	return true;
}

static AllegroTimerApi * s_timerApi = nullptr;

static thread_local AllegroVoiceAPI * voiceAPI = nullptr;

extern "C"
{
static AudioOutput_PortAudio * audioOutput = nullptr;
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
	if (voiceAPI != nullptr)
		return -1;
	
	voiceAPI = new AllegroVoiceAPI(DIGI_SAMPLERATE);
	
	audioOutput = new AudioOutput_PortAudio();
	audioOutput->Initialize(2, DIGI_SAMPLERATE, 64);
	
	audioStream = new AudioStream_AllegroVoiceMixer(voiceAPI);
	audioStream->timerAPI = s_timerApi;
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

#include <unistd.h>

static int TimerThreadProc(void * obj)
{
	AllegroTimerReg * r = (AllegroTimerReg*)obj;
	
	// todo : use POSIX timer API ?
	
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
	
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	
	while (r->stop == false)
	{
		r->proc(r->data);
		
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

AudioStream_AllegroVoiceMixer::AudioStream_AllegroVoiceMixer(AllegroVoiceAPI * in_voiceAPI)
	: AudioStream()
	, voiceAPI(in_voiceAPI)
	, timerAPI(nullptr)
{
}

int AudioStream_AllegroVoiceMixer::Provide(int numSamples, AudioSample* __restrict buffer)
{
	if (timerAPI != nullptr && timerAPI->mode == AllegroTimerApi::kMode_Manual)
	{
		::voiceAPI = this->voiceAPI;
		timerAPI->processInterrupts(numSamples);
		::voiceAPI = nullptr;
	}
	
	int * __restrict mixingBuffer = (int*)alloca(numSamples * 2 * sizeof(int));
	memset(mixingBuffer, 0, numSamples * 2 * sizeof(int));
	
	voiceAPI->lock();
	{
	#if 0
		float * __restrict samplesL = (float*)alloca(numSamples * sizeof(float));
		float * __restrict samplesR = (float*)alloca(numSamples * sizeof(float));
		
		memset(samplesL, 0, numSamples * sizeof(float));
		memset(samplesR, 0, numSamples * sizeof(float));
		
		float * __restrict samplesV = (float*)alloca(numSamples * sizeof(float));
		
		for (int i = 0; i < AllegroVoiceAPI::MAX_VOICES; ++i)
		{
			float stereoPanning;
			
			if (voiceAPI->generateSamplesForVoice(i, samplesV, numSamples, stereoPanning))
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
			buffer[i].channel[0] = samplesL[i] * (1 << (15 - 2));
			buffer[i].channel[1] = samplesR[i] * (1 << (15 - 2));
		}
	#else
		for (auto & __restrict voice : voiceAPI->voices)
		{
			if (voice.used && voice.started && voice.sample->len != 0)
			{
				// todo : implement panning separation
				//const int pan = 127 + ((voice.pan - 127) >> 2);
				
				const int pan = voice.pan;
				
				const int pan1 = (0xff - pan) * voice.volume;
				const int pan2 = (       pan) * voice.volume;
				
				int sampleIndex = voice.position >> FIXBITS;
				
				for (int i = 0; i < numSamples; ++i)
				{
					Assert(sampleIndex >= 0 && sampleIndex < voice.sample->len);
					
					if (sampleIndex >= 0 && sampleIndex < voice.sample->len)
					{
					#if INTERP_LINEAR
						const int sampleIndex2 = sampleIndex + 1 < voice.sample->len ? sampleIndex + 1 : sampleIndex;
						
						const int t = (voice.position >> (FIXBITS - 16)) & 0xffff;
					#endif
					
						if (voice.sample->bits == 8)
						{
							const unsigned char * __restrict values = (unsigned char*)voice.sample->data;
							
						#if INTERP_LINEAR
							const int value1 = int8_t(values[sampleIndex ] ^ 0x80);
							const int value2 = int8_t(values[sampleIndex2] ^ 0x80);
							
							const int value = (value1 * (0xffff - t) + value2 * t) >> 16;
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
							
							const int value = (value1 * (0xffff - t) + value2 * t) >> 16;
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
	voiceAPI->unlock();
	
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
