#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"
#include "jgvis.h"

#define bitmap_height 12

struct OLD_CHN_INFO
{
    int old_sample;
    Color color;
};

static void drawCircle(const JGMOD_PLAYER & player, const int chn, const int start_chn, const int note_length);

// todo : remove this global state!

static OLD_CHN_INFO old_chn_info[JGMOD_MAX_VOICES];

void jgmod_draw(const JGMOD_PLAYER & player, const bool drawCircles)
{
	setColor(colorWhite);
	
	AllegroVoiceApi * voiceApi = player.voiceApi;
	
	const JGMOD * mod = player.of;
	
	if (mod == nullptr)
	{
		// draw header
		
		drawText(0,  0, 12, +1, +1, "Song name   : %s", "Load error");
	}
	else
	{
		int start_chn = 0;
		int end_chn = mod->no_chn;
		
		if (end_chn > 33)
			end_chn = 33;
		
		int note_length;
		
		if (mod->flag & JGMOD_MODE_XM)
			note_length = 180;
		else
			note_length = 140;
		
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
			if (old_chn_info[index].old_sample != player.ci[index].sample)
				old_chn_info[index].color = Color::fromHSL((rand() % 68 + 32) / 100.f, .5f, .5f);

			old_chn_info[index].old_sample = player.ci[index].sample;
		}

		for (int index = start_chn; index < end_chn; ++index)
		{
			if (drawCircles)
			{
				drawCircle(player, index, start_chn, note_length);
			}

			if (voiceApi->voice_get_position(player.voice_table[index]) >= 0 &&
				player.ci[index].volume >= 1 &&
				player.ci[index].volenv.v >= 1 &&
				voiceApi->voice_get_frequency(player.voice_table[index]) > 0 &&
				player.mi.global_volume > 0)
			{
				drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3d %2d %6dHz %3d : %s", index+1, player.ci[index].sample+1, player.ci[index].volume,  voiceApi->voice_get_frequency(player.voice_table[index]), player.ci[index].pan,
					player.of->si[player.ci[index].sample].name);
			}
			else
			{
				drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---");
			}
		}
	}
}

static void drawCircle(const JGMOD_PLAYER & player, const int chn, const int start_chn, const int note_length)
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
			//xpos += note_relative_pos;

			if (xpos > 439)
				xpos = 439;
			else if (xpos < 0)
				xpos = 0;
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(old_chn_info[chn].color);
				hqFillCircle(xpos, 5, radius);
				setColor(colorWhite);
			}
			hqEnd();
		}
		gxPopMatrix();
	}
}