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
 *  New effects for s3m are located here. */

#include "jgmod.h"
#include "jshare.h"

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"

#include <algorithm>
#include <math.h> // pow

#define speed_ratio     mi.speed_ratio / 100
#define pitch_ratio     mi.pitch_ratio / 100

void JGMOD_PLAYER::do_s3m_set_tempo(const int extcommand)
{
    if (extcommand > 0)
		mi.tempo = extcommand;
}

void JGMOD_PLAYER::do_s3m_set_bpm(const int extcommand)
{
    if (extcommand >= 32)
        mi.bpm = extcommand;
    else if (extcommand <= 15)
        mi.bpm += extcommand;
    else if ( (extcommand > 15) && (extcommand < 32) )
        mi.bpm -= extcommand - 16;

    timerApi->install_int_ex2(
    	mod_interrupt_proc,
    	BPM_TO_TIMER(mi.bpm * 24 * speed_ratio),
    	this);
}

void JGMOD_PLAYER::parse_s3m_volume_slide(const int chn, const int extcommand)
{
    // fine volume slide down
    if ((extcommand & 0xF0) == 0xF0 && (extcommand & 0xF) != 0)
	{
        ci[chn].s3m_fine_volume_slide = -(extcommand & 0xF);
        ci[chn].s3m_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = true;
	}
    // fine volume slide up
    else if ((extcommand & 0xF) == 0xF && (extcommand & 0xF0) != 0)
	{
        ci[chn].s3m_fine_volume_slide = ((extcommand & 0xF0) >> 4);
        ci[chn].s3m_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = true;
	}
    // volume slide up
    else if ((extcommand & 0xF) == 0 && (extcommand & 0xF0) != 0)
	{
        ci[chn].s3m_volume_slide = extcommand >> 4;
        ci[chn].s3m_fine_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = true;
	}
    // volume slide down
    else if ((extcommand & 0xF0) == 0 && (extcommand & 0xF) != 0)
	{
        ci[chn].s3m_volume_slide = -(extcommand & 0xF);
        ci[chn].s3m_fine_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = true;
	}
	// continue volume slide
    else if (extcommand == 0)
    {
        ci[chn].s3m_volume_slide_on = true;
	}
}

void JGMOD_PLAYER::do_s3m_volume_slide(const int chn)
{
    if (mi.tick == 0) // finetunes
	{
        ci[chn].volume += ci[chn].s3m_fine_volume_slide;
        ci[chn].temp_volume += ci[chn].s3m_fine_volume_slide;        
	}
    else // volume slide
	{
        ci[chn].volume += ci[chn].s3m_volume_slide;
        ci[chn].temp_volume += ci[chn].s3m_volume_slide;        
	}
}

void JGMOD_PLAYER::parse_s3m_portamento_up(const int chn, const int extcommand)
{
    if (!ci[chn].period)
        return;

    if (extcommand > 0 && extcommand <= 0xDF)
	{
        ci[chn].s3m_fine_pitch_slide = 0;
		
		if (of->flag & JGMOD_MODE_IT)
			ci[chn].s3m_pitch_slide = -extcommand * 2;
		else
        	ci[chn].s3m_pitch_slide = -extcommand * 4;
	}
    else if (extcommand >= 0xE0 && extcommand <= 0xEF)
	{
        ci[chn].s3m_fine_pitch_slide = -(extcommand & 0xF);
        ci[chn].s3m_pitch_slide = 0;
	}
    else if (extcommand >= 0xF0 && extcommand <= 0xFF)
	{
        ci[chn].s3m_fine_pitch_slide = -((extcommand & 0xF) << 2);
        ci[chn].s3m_pitch_slide = 0;
	}
    else if (extcommand == 0)
	{
        ci[chn].s3m_pitch_slide = -std::abs(ci[chn].s3m_pitch_slide);
        ci[chn].s3m_fine_pitch_slide = -std::abs(ci[chn].s3m_fine_pitch_slide);
	}

    ci[chn].s3m_pitch_slide_on = true;
}

void JGMOD_PLAYER::parse_s3m_portamento_down(const int chn, const int extcommand)
{
    if (!ci[chn].period)
        return;

    if (extcommand > 0 && extcommand <= 0xDF)
	{
        ci[chn].s3m_fine_pitch_slide = 0;
		
		if (of->flag & JGMOD_MODE_IT)
			ci[chn].s3m_pitch_slide = extcommand * 2;
		else
        	ci[chn].s3m_pitch_slide = extcommand * 4;
	}
    else if (extcommand >= 0xE0 && extcommand <= 0xEF)
	{
        ci[chn].s3m_pitch_slide = 0;
        ci[chn].s3m_fine_pitch_slide = extcommand & 0xF;
	}
    else if (extcommand >= 0xF0 && extcommand <= 0xFF)
	{
        ci[chn].s3m_pitch_slide = 0;
        ci[chn].s3m_fine_pitch_slide = ((extcommand & 0xF) << 2);
	}
	else if (extcommand == 0)
	{
        ci[chn].s3m_pitch_slide = std::abs(ci[chn].s3m_pitch_slide);
        ci[chn].s3m_fine_pitch_slide = std::abs(ci[chn].s3m_fine_pitch_slide);
     }

     ci[chn].s3m_pitch_slide_on = true;
}

static int linear_slide(int period, int slide)
{
#if 1
	double frequency = 1.0 / period;
	
	frequency = frequency * pow(2.0, slide / 768.0);
	
	return 1.0 / frequency;
#else
	const double M = 512.0;
	
	double frequency = 1.0 / period;
	double slide_in_hz = frequency * slide / M;
	frequency -= slide_in_hz;
	if (frequency <= 0.0)
		return 0;
	else
		return 1.0 / frequency;
#endif
}

void JGMOD_PLAYER::do_s3m_portamento(const int chn)
{
    if (mi.tick == 0)
	{
		// fine tunings
		if ((mi.flag & JGMOD_MODE_IT) && (mi.flag & JGMOD_MODE_LINEAR))
		{
			ci[chn].period = linear_slide(ci[chn].period, ci[chn].s3m_fine_pitch_slide);
			ci[chn].temp_period = linear_slide(ci[chn].temp_period, ci[chn].s3m_fine_pitch_slide);
		}
		else
		{
        	ci[chn].period += ci[chn].s3m_fine_pitch_slide;
        	ci[chn].temp_period += ci[chn].s3m_fine_pitch_slide;
		}
	}
    else
	{
		// pitch slide
		if ((mi.flag & JGMOD_MODE_IT) && (mi.flag & JGMOD_MODE_LINEAR))
		{
			ci[chn].period = linear_slide(ci[chn].period, ci[chn].s3m_pitch_slide);
			ci[chn].temp_period = linear_slide(ci[chn].temp_period, ci[chn].s3m_pitch_slide);
		}
		else
		{
        	ci[chn].period += ci[chn].s3m_pitch_slide;
        	ci[chn].temp_period += ci[chn].s3m_pitch_slide;
		}
	}
}

void JGMOD_PLAYER::parse_s3m_arpeggio(const int chn, const int extcommand)
{
    if (!ci[chn].period)
        return;

    ci[chn].arpeggio_on = true;
	
    if (extcommand)
        ci[chn].arpeggio = extcommand;
}

void JGMOD_PLAYER::parse_tremor(const int chn, const int extcommand)
{
    const int on  = (extcommand >> 4) + 1;
    const int off = (extcommand & 0xF) + 1;

    ci[chn].tremor_set = (on << 8) + off;
    ci[chn].tremor_on = true;
}

void JGMOD_PLAYER::do_tremor(const int chn)
{
    const int on  = (ci[chn].tremor_set >> 8) & 31;
    const int off = ci[chn].tremor_set & 31;

    ci[chn].tremor_count %= (on + off);

    if (ci[chn].tremor_count < on)
        ci[chn].temp_volume = ci[chn].volume;
    else
        ci[chn].temp_volume = 0;
        
    ci[chn].tremor_count++;
}

void JGMOD_PLAYER::do_global_volume(const int extcommand)
{
    mi.global_volume = extcommand;

    if (mi.global_volume > 64)
        mi.global_volume = 64;
}

void JGMOD_PLAYER::parse_s3m_panning(const int chn, const int extcommand)
{
    if (extcommand == 0)
        ci[chn].pan = 0;
    else if (extcommand <= 0x80)
        ci[chn].pan = (extcommand << 1) - 1;
}

void JGMOD_PLAYER::parse_s3m_retrig(const int chn, const int extcommand)
{
    if (extcommand)
	{
        ci[chn].s3m_retrig = extcommand & 0xF;
        ci[chn].s3m_retrig_slide = ((extcommand & 0xF0) >> 4);
	}

    ci[chn].s3m_retrig_on = true;
}

void JGMOD_PLAYER::do_s3m_retrig(const int chn)
{
    if (ci[chn].s3m_retrig > 0)
	{
        if ((mi.tick % ci[chn].s3m_retrig) == 0)
		{
            ci[chn].kick = true;
                
            switch (ci[chn].s3m_retrig_slide)
			{
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				{
					ci[chn].temp_volume -=  ( 1 << (ci[chn].s3m_retrig_slide - 1) );
					ci[chn].volume -=  ( 1 << (ci[chn].s3m_retrig_slide - 1) );
					break;
				}
			case 6:
				{
					ci[chn].temp_volume  = (2 * ci[chn].temp_volume) / 3;
					ci[chn].volume  = (2 * ci[chn].volume) / 3;
					break;
				}
			case 7:
				{
					ci[chn].temp_volume =  (ci[chn].temp_volume >> 1);
					ci[chn].volume =  (ci[chn].volume >> 1);
					break;
				}
			case 9:
			case 0xA:
			case 0xB:
			case 0xC:
			case 0xD:
				{
					ci[chn].temp_volume += (1 << (ci[chn].s3m_retrig_slide - 9));
					ci[chn].volume += (1 << (ci[chn].s3m_retrig_slide - 9));
					break;
				}
			case 0xE:
				{
					ci[chn].temp_volume = (3 * ci[chn].temp_volume) / 2;
					ci[chn].volume = (3 * ci[chn].volume) / 2;
					break;
				}
			case 0xF:
				{
					ci[chn].temp_volume = ci[chn].temp_volume << 1;
					ci[chn].volume = ci[chn].volume << 1;
					break;
				}
			}
		}
	}
}

void JGMOD_PLAYER::parse_volume_command(const int chn, const int volume, const int note)
{
    if (volume >= 0x10 && volume <= 0x50)         // set volume
    {
        ci[chn].volume = volume - 0x10;
	}
    else if (volume >= 0x60 && volume <= 0x6F)    // volume slide down
    {
        ci[chn].pro_volume_slide = -(volume & 0xF);
	}
    else if (volume >= 0x70 && volume <= 0x7F)    // volume slide up
    {
        ci[chn].pro_volume_slide = (volume & 0xF);
	}
    else if (volume >= 0x80 && volume <= 0x8F)    // fine vol slide down
    {
        ci[chn].volume -=  (volume & 0xF);
	}
    else if (volume >= 0x90 && volume <= 0x9F)    // fine vol slide up
	{
        ci[chn].volume +=  (volume & 0xF);
	}
    else if (volume >= 0xA0 && volume <= 0xAF)    // vibrato speed
    {
        ci[chn].vibrato_speed = ((volume & 0xF) << 2);
	}
    else if (volume >= 0xB0 && volume <= 0xBF)    // do vibrato
    {
        parse_vibrato (chn, (volume & 0xF), 5);
	}
    else if (volume >= 0xC0 && volume <= 0xCF)    // set panning
    {
        ci[chn].pan = (volume & 0xF) * 17;
	}
    else if (volume >= 0xD0 && volume <= 0xDF)    // panning slide left
	{
        if (volume & 0xF)
            ci[chn].pan_slide_left = -(volume & 0xF);

        ci[chn].pan_slide_common = ci[chn].pan_slide_left;
	}
    else if (volume >= 0xE0 && volume <= 0xEF)    // panning slide right
	{
        if (volume & 0xF)
            ci[chn].pan_slide_right = (volume & 0xF);

        ci[chn].pan_slide_common = ci[chn].pan_slide_right;
	}
    else if (volume >= 0xF0 && volume <= 0xFF)    // porta to note
    {
        parse_slide2period(chn, (volume & 0xF) << 4, (note > 0) ? note + ci[chn].transpose : 0);
	}
	
	//

    if (ci[chn].volume > 64)
        ci[chn].volume = 64;
    else if (ci[chn].volume < 0)
        ci[chn].volume = 0;
}

void JGMOD_PLAYER::parse_note_command(const int chn, const int note)
{
    if (note == -1)
        ci[chn].volume = 0;

    if (note == -2)
	{
        ci[chn].keyon = true;
		
        if ((ci[chn].volenv.flg & JGMOD_ENV_ON) == 0)
            ci[chn].volume = 0;            
	}
	
    if (note == -3)
	{
        ci[chn].keyon = false;
	}
}
