/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "audiostream/AudioOutput_PortAudio.h"
#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"
#include "vfxNodeJgmod.h"

// todo : remove install_sound etc and manage audio output ourselves. requires refactoring of jgmod voice management

#define DIGI_SAMPLERATE 192000

VFX_NODE_TYPE(VfxNodeJgmod)
{
	typeName = "jgmod";
	
	in("filename", "string");
	in("autoplay", "bool", "1");
	in("loop", "bool", "1");
	in("gain", "float", "1");
	in("speed", "float", "1");
	in("pitch", "float", "1");
	in("play!", "trigger");
	in("pause!", "trigger");
	in("resume!", "trigger");
	in("prevTrack!", "trigger");
	in("nextTrack!", "trigger");
	out("track", "float");
	out("pattern", "float");
	out("row", "float");
	out("end!", "trigger");
}

VfxNodeJgmod::VfxNodeJgmod()
	: VfxNodeBase()
	, currentFilename()
	, mod(nullptr)
	, player(nullptr)
	, isPlaying(false)
	, trackOutput(0.f)
	, patternOutput(0.f)
	, rowOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_Autoplay, kVfxPlugType_Bool);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_Gain, kVfxPlugType_Float);
	addInput(kInput_Speed, kVfxPlugType_Float);
	addInput(kInput_Pitch, kVfxPlugType_Float);
	addInput(kInput_Play, kVfxPlugType_Trigger);
	addInput(kInput_Pause, kVfxPlugType_Trigger);
	addInput(kInput_Resume, kVfxPlugType_Trigger);
	addInput(kInput_PrevTrack, kVfxPlugType_Trigger);
	addInput(kInput_NextTrack, kVfxPlugType_Trigger);
	addOutput(kOutput_Track, kVfxPlugType_Float, &trackOutput);
	addOutput(kOutput_Pattern, kVfxPlugType_Float, &patternOutput);
	addOutput(kOutput_Row, kVfxPlugType_Float, &rowOutput);
	addOutput(kOutput_End, kVfxPlugType_Trigger, nullptr);
	
// todo : move allegro init elsewhere, or remove the need for init altogether by abstracting the mixer

	timerApi = new AllegroTimerApi(AllegroTimerApi::kMode_Manual);
	voiceApi = new AllegroVoiceApi(DIGI_SAMPLERATE, true);
	
	audioStream = new AudioStream_AllegroVoiceMixer(voiceApi);
	audioStream->timerApi = timerApi;
	
// todo : to make sure audio nodes added 'at the same time' (graph construction) are perfectly in sync, audio streaming should be paused globally, to avoid things from triggering ahead of things, before all streams are registered. this pleads for a global vfx graph controlled audio stream

	audioOutput = new AudioOutput_PortAudio();
	audioOutput->Initialize(2, DIGI_SAMPLERATE, 64);
	audioOutput->Play(audioStream);

	player = new JGMOD_PLAYER();
	player->init(JGMOD_MAX_VOICES, timerApi, voiceApi);
}

VfxNodeJgmod::~VfxNodeJgmod()
{
	free();
	
	delete player;
	player = nullptr;
	
	delete audioOutput;
	audioOutput = nullptr;
	
	delete audioStream;
	audioStream = nullptr;
	
	delete voiceApi;
	voiceApi = nullptr;
	
	delete timerApi;
	timerApi = nullptr;
}

void VfxNodeJgmod::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeJgmod);
	
	if (isPassthrough)
	{
		free();
		return;
	}
	
	const char * filename = getInputString(kInput_Filename, nullptr);
	const bool autoplay = getInputBool(kInput_Autoplay, true);
	const bool loop = getInputBool(kInput_Loop, true);
	const float gain = getInputFloat(kInput_Gain, 1.f);
	const float speed = getInputFloat(kInput_Speed, 1.f);
	const float pitch = getInputFloat(kInput_Pitch, 1.f);
	
	if (filename == nullptr)
	{
		free();
	}
	else if (filename != currentFilename)
	{
		free();
		
		//
		
		currentFilename = filename;
		
		load(filename);
		
		if (autoplay)
		{
			player->play(mod, loop, 100, 100);
			
			isPlaying = true;
		}
	}
	
	if (player->is_playing())
	{
		player->set_loop(loop);
		player->set_volume(gain * 255.f);
		player->set_speed(speed * 100.f);
		player->set_pitch(pitch * 100.f);
	}
	
	if (player->is_playing())
	{
		trackOutput =  player->mi.trk;
		patternOutput = player->mi.pat;
		rowOutput =  player->mi.pos;
	}
	else
	{
		if (isPlaying)
		{
			isPlaying = false;
			
			trigger(kOutput_End);
		}
		
		trackOutput = 0;
		patternOutput = 0;
		rowOutput = 0;
	}
}

void VfxNodeJgmod::handleTrigger(const int index)
{
	if (index == kInput_Play)
	{
		if (mod != nullptr)
		{
			const bool loop = getInputBool(kInput_Loop, true);
			
			player->play(mod, loop, 100, 100);
			
			isPlaying = true;
		}
	}
	else if (index == kInput_Pause)
	{
		if (player->is_playing() && !player->is_paused())
			player->pause();
	}
	else if (index == kInput_Resume)
	{
		if (player->is_playing() && player->is_paused())
			player->resume();
	}
	else if (index == kInput_NextTrack)
	{
		if (player->is_playing())
			player->next_track();
	}
	else if (index == kInput_PrevTrack)
	{
		if (player->is_playing())
			player->prev_track();
	}
}

void VfxNodeJgmod::getDescription(VfxNodeDescription & d)
{
	d.add("filename: %s", currentFilename.c_str());
	
	if (mod == nullptr)
	{
		// header
		
		d.add("Song name   : %s", "Load error");
	}
	else
	{
		// header
		
		d.add("Song name   : %s", mod->name);
		d.add("No Channels : %2d  Period Type : %s  No Inst : %2d ",
			mod->no_chn, (mod->flag & JGMOD_MODE_LINEAR) ? "Linear" : "Amiga", mod->no_instrument);
		d.add("No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", mod->no_trk, mod->no_pat, mod->no_sample);
		
		// playback info
		
		d.add("Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ",
			player->mi.tempo,
			player->mi.bpm,
			player->mi.speed_ratio,
			player->mi.pitch_ratio);
		d.add("Global volume : %2d  User volume : %2d ",
			player->mi.global_volume,
			player->get_volume());
		d.add("%03d-%02d-%02d    ",
			player->mi.trk,
			player->mi.pos,
			player->mi.tick < 0 ? 0 : player->mi.tick);
		
		for (int index = 0; index < mod->no_chn; ++index)
		{
			if (voiceApi->voice_get_position(player->voice_table[index]) >= 0 &&
				player->ci[index].volume >= 1 &&
				player->ci[index].volenv.v >= 1 &&
				voiceApi->voice_get_frequency(player->voice_table[index]) > 0 &&
				player->mi.global_volume > 0)
			{
				d.add("%2d: %3d %2d %6dHz %3d ",
					index+1,
					player->ci[index].sample+1,
					player->ci[index].volume,
					voiceApi->voice_get_frequency(player->voice_table[index]),
					player->ci[index].pan);
			}
			else
			{
				d.add("%2d: %3s %2s %6sHz %3s  ",
					index+1,
					" --", "--",  " -----", "---");
			}
		}
	}
}

void VfxNodeJgmod::load(const char * filename)
{
	Assert(mod == nullptr);
	Assert(player->of == nullptr);
	
	mod = jgmod_load(filename, false, true);
}

void VfxNodeJgmod::free()
{
	if (player->of != nullptr)
	{
		// todo : stop should clear 'of' member of player, making destroy_mod redundant ?
		
		player->stop();
		
		player->destroy_mod();
		
		mod = nullptr;
	}
	else if (mod != nullptr)
	{
		jgmod_destroy(mod);
		
		mod = nullptr;
	}
	
	currentFilename.clear();
}
