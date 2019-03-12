/*
 *
 *                _______  _______  __________  _______  _____
 *               /____  / / _____/ /         / / ___  / / ___ \
 *               __  / / / / ____ / //   // / / /  / / / /  / /
 *             /  /_/ / / /__/ / / / /_/ / / / /__/ / / /__/ /
 *            /______/ /______/ /_/     /_/ /______/ /______/
 *
 *
 *
 *
 *  Visualisation and player interaction. */

#include "allegro2-voiceApi.h"
#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"
#include "jgvis.h"

#define bitmap_height 12

static void drawCircle(const JGVIS & vis, const JGMOD_PLAYER & player, const int chn, const int start_chn, const int note_length, const int note_relative_pos);

static int calculate_note_length(const JGMOD * mod, int note_length)
{
	if (note_length == 0)
	{
		if (mod->flag & JGMOD_MODE_XM)
			note_length = 180;
		else
			note_length = 140;
	}
	
	return note_length;
}

static int normalize_end_chn(JGMOD * mod, int end_chn)
{
	if (end_chn < 0)
	{
		end_chn = mod->no_chn;
		
		if (end_chn > 33)
        	end_chn = 33;
	}
	
	return end_chn;
}

void jgvis_tick(JGVIS & vis, JGMOD_PLAYER & player, bool & inputIsCaptured)
{
	if (inputIsCaptured)
		return;
	
	inputIsCaptured = true;
	
	const JGMOD * mod = player.of;
	
	vis.note_length = calculate_note_length(player.of, vis.note_length);
	vis.end_chn = normalize_end_chn(player.of, vis.end_chn);
	
	if (keyboard.wentDown(SDLK_LEFT, true))
		player.prev_track();
	if (keyboard.wentDown(SDLK_RIGHT, true))
		player.next_track();

	if (keyboard.wentDown(SDLK_PLUS, true))
		player.set_volume(player.get_volume() + 5);
	if (keyboard.wentDown(SDLK_MINUS, true))
		player.set_volume(player.get_volume() - 5);

	if (keyboard.wentDown(SDLK_F1, true))
		player.set_speed (player.mi.speed_ratio - 5);
	if (keyboard.wentDown(SDLK_F2, true))
		player.set_speed (player.mi.speed_ratio + 5);
	if (keyboard.wentDown(SDLK_F3, true))
		player.set_pitch (player.mi.pitch_ratio - 5);
	if (keyboard.wentDown(SDLK_F4, true))
		player.set_pitch (player.mi.pitch_ratio + 5);
	if (keyboard.wentDown(SDLK_F5, true))
		vis.note_length++;

	if (keyboard.wentDown(SDLK_F6, true))
	{
		vis.note_length--;
		if (vis.note_length <= 0)
			vis.note_length = 1;
	}
	if (keyboard.wentDown(SDLK_F7, true))
	{
		vis.note_relative_pos -= 2;
		if (vis.note_relative_pos < -300)
			vis.note_relative_pos = -300;
	}
	if (keyboard.wentDown(SDLK_F8, true))
	{
		vis.note_relative_pos += 2;
		if (vis.note_relative_pos > 300)
			vis.note_relative_pos = 300;
	}

	if (keyboard.wentDown(SDLK_r, true))
		player.play (player.of, true);
	if (keyboard.wentDown(SDLK_p, true) || keyboard.wentDown(SDLK_SPACE, true))
		player.toggle_pause_mode ();

	if (keyboard.wentDown(SDLK_DOWN, true))
	{
		if (mod->no_chn > 33)
		{
			vis.end_chn = vis.start_chn + 33 + 1;
			if (vis.end_chn > mod->no_chn)
				vis.end_chn = mod->no_chn;

			vis.start_chn = vis.end_chn - 33;
		}
	}
	if (keyboard.wentDown(SDLK_UP, true))
	{
		if (mod->no_chn > 33)
		{
			vis.start_chn--;
			if (vis.start_chn < 0)
				vis.start_chn = 0;

			vis.end_chn = vis.start_chn + 33;
		}
	}
}

void jgvis_draw(JGVIS & vis, const JGMOD_PLAYER & player, const bool drawCircles)
{
	setColor(colorWhite);
	
	AllegroVoiceApi * voiceApi = player.voiceApi;
	
	const JGMOD * mod = player.of;
	
	//
	
	vis.note_length = calculate_note_length(player.of, vis.note_length);
	vis.end_chn = normalize_end_chn(player.of, vis.end_chn);
	
	//
	
	if (mod == nullptr)
	{
		// draw header
		
		drawText(0,  0, 12, +1, +1, "Song name   : %s", "Load error");
	}
	else
	{
		// draw header
		
		drawText(0,  0, 12, +1, +1, "Song name   : %s", mod->name);
		drawText(0, 12, 12, +1, +1, "No Channels : %2d  Period Type : %s  No Inst : %2d ",
			mod->no_chn, (mod->flag & JGMOD_MODE_LINEAR) ? "Linear" : "Amiga", mod->no_instrument);
		drawText(0, 24, 12, +1, +1, "No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", mod->no_trk, mod->no_pat, mod->no_sample);
		
		// draw playback info
		
		drawText(0, 36, 12, +1, +1, "Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ", player.mi.tempo, player.mi.bpm, player.mi.speed_ratio, player.mi.pitch_ratio);
		drawText(0, 48, 12, +1, +1, "Global volume : %2d  Mixing volume : %2d  User volume : %2d ", player.mi.global_volume, player.of->mixing_volume, player.get_volume());
		drawText(0, 70, 12, +1, +1, "%03d-%02d-%02d (%02d)", player.mi.trk, player.mi.pos, player.mi.tick < 0 ? 0 : player.mi.tick, player.mi.pat);

		for (int index = 0; index < mod->no_chn; ++index)
		{
			if (vis.old_chn_info[index].old_sample != player.ci[index].sample)
				vis.old_chn_info[index].color = Color::fromHSL((rand() % 68 + 32) / 100.f, .5f, .5f);

			vis.old_chn_info[index].old_sample = player.ci[index].sample;
		}

		for (int index = vis.start_chn; index < vis.end_chn; ++index)
		{
			if (drawCircles)
			{
				drawCircle(vis, player, index, vis.start_chn, vis.note_length, vis.note_relative_pos);
			}

			if (voiceApi->voice_get_position(player.voice_table[index]) >= 0 &&
				player.ci[index].volume >= 1 &&
				player.ci[index].volenv.v >= 1 &&
				voiceApi->voice_get_frequency(player.voice_table[index]) > 0 &&
				player.mi.global_volume > 0)
			{
				drawText(0, 82+(index-vis.start_chn)*bitmap_height, 12, +1, +1, "%2d: %3d %2d %6dHz %3d : %s", index+1, player.ci[index].sample+1, player.ci[index].volume,  voiceApi->voice_get_frequency(player.voice_table[index]), player.ci[index].pan,
					player.of->si[player.ci[index].sample].name);
			}
			else
			{
				drawText(0, 82+(index-vis.start_chn)*bitmap_height, 12, +1, +1, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---");
			}
		}
	}
}

static void drawCircle(const JGVIS & vis, const JGMOD_PLAYER & player, const int chn, const int start_chn, const int note_length, const int note_relative_pos)
{
    if (player.voiceApi->voice_get_position(player.voice_table[chn]) >= 0 &&
    	player.ci[chn].volume >= 1 &&
    	player.ci[chn].volenv.v >= 1 &&
		player.voiceApi->voice_get_frequency(player.voice_table[chn]) > 0 &&
    	(player.mi.global_volume > 0))
	{
		gxPushMatrix();
		{
			const int dx = 200;
			const int dy = 79+(chn-start_chn)*bitmap_height;
			gxTranslatef(dx, dy, 0);
			
			const float radius = (player.ci[chn].volume / 13.f) + 2.f;
			
			float xpos;
			
			if (player.mi.flag & JGMOD_MODE_XM)
				xpos = player.voiceApi->voice_get_frequency(player.voice_table[chn]);
			else
				xpos = player.voiceApi->voice_get_frequency(player.voice_table[chn]) * 8363.f / player.ci[chn].c2spd;

			xpos /= note_length;
			xpos += note_relative_pos;

			if (xpos > 439)
				xpos = 439;
			else if (xpos < 0)
				xpos = 0;
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(vis.old_chn_info[chn].color);
				hqFillCircle(xpos, 5, radius);
				setColor(colorWhite);
			}
			hqEnd();
		}
		gxPopMatrix();
	}
}
