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

#include "framework-allegro2.h"
#include "jgmod.h"
#include "jshare.h"


#define speed_ratio     mi.speed_ratio / 100
#define pitch_ratio     mi.pitch_ratio / 100

void do_s3m_set_tempo (int extcommand)
{
    if (extcommand > 0)
        mi.tempo = extcommand;

}END_OF_FUNCTION (do_s3m_set_tempo)

void do_s3m_set_bpm (int extcommand)
{
    if (extcommand >= 32)
        mi.bpm = extcommand;
    else if (extcommand <= 15)
        mi.bpm += extcommand;
    else if ( (extcommand > 15) && (extcommand < 32) )
        mi.bpm -= extcommand - 16;

    //remove_int (mod_interrupt);
    install_int_ex (mod_interrupt, BPM_TO_TIMER (mi.bpm * 24 * speed_ratio));
}END_OF_FUNCTION (do_s3m_set_bpm)

void parse_s3m_volume_slide (int chn, int extcommand)
{
    // fine volume slide down
    if ( (extcommand & 0xF0) == 0xF0  && (extcommand & 0xF) != 0) 
        {
        ci[chn].s3m_fine_volume_slide = -(extcommand & 0xF);
        ci[chn].s3m_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = TRUE;
        }

    // fine volume slide up
    else if ( (extcommand & 0xF) == 0xF && (extcommand & 0xF0) != 0)
        {
        ci[chn].s3m_fine_volume_slide = ((extcommand & 0xF0) >> 4);
        ci[chn].s3m_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = TRUE;
        }

    // volume slide up
    else if ( (extcommand & 0xF) == 0  && (extcommand & 0xF0) != 0)
        {
        ci[chn].s3m_volume_slide = extcommand >> 4;
        ci[chn].s3m_fine_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = TRUE;
        }

    // volume slide down
    else if ( (extcommand & 0xF0) == 0  && (extcommand & 0xF) != 0) 
        {
        ci[chn].s3m_volume_slide = -(extcommand & 0xF);
        ci[chn].s3m_fine_volume_slide = 0;
        ci[chn].s3m_volume_slide_on = TRUE;
        }
    else if (extcommand == 0)          // continue volume slide 
        ci[chn].s3m_volume_slide_on = TRUE;

}END_OF_FUNCTION (parse_s3m_volume_slide);

void do_s3m_volume_slide (int chn)
{
    if (mi.tick == 0)   //finetunes
        {
        ci[chn].volume += ci[chn].s3m_fine_volume_slide;
        ci[chn].temp_volume += ci[chn].s3m_fine_volume_slide;        
        }
    else                // volume slide
        {
        ci[chn].volume += ci[chn].s3m_volume_slide;
        ci[chn].temp_volume += ci[chn].s3m_volume_slide;        
        }

}END_OF_FUNCTION (do_s3m_volume_slide);

void parse_s3m_portamento_up (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    if ( (extcommand > 0) && (extcommand <= 0xDF) )
        {
        ci[chn].s3m_fine_pitch_slide = 0;
        ci[chn].s3m_pitch_slide = -extcommand * 4;
        }
    else if ( (extcommand >= 0xE0) && (extcommand <= 0xEF) )
        {
        ci[chn].s3m_fine_pitch_slide = -(extcommand & 0xF);
        ci[chn].s3m_pitch_slide = 0;
        }
    else if ( (extcommand >= 0xF0) && (extcommand <= 0xFF) )
        {
        ci[chn].s3m_fine_pitch_slide = -((extcommand & 0xF) << 2);
        ci[chn].s3m_pitch_slide = 0;
        }
    else if (extcommand == 0)
        {
        ci[chn].s3m_pitch_slide = -ABS(ci[chn].s3m_pitch_slide);
        ci[chn].s3m_fine_pitch_slide = -ABS(ci[chn].s3m_fine_pitch_slide);
        }

    ci[chn].s3m_pitch_slide_on = TRUE;

}END_OF_FUNCTION (parse_s3m_portamento_up)

void parse_s3m_portamento_down (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    if ( (extcommand > 0) && (extcommand <= 0xDF) )
        {
        ci[chn].s3m_fine_pitch_slide = 0;
        ci[chn].s3m_pitch_slide = extcommand * 4;
        }
    else if ( (extcommand >= 0xE0) && (extcommand <= 0xEF))
        {
        ci[chn].s3m_pitch_slide = 0;
        ci[chn].s3m_fine_pitch_slide = extcommand & 0xF;
        }
    else if ( (extcommand >= 0xF0) && (extcommand <= 0xFF))
        {
        ci[chn].s3m_pitch_slide = 0;
        ci[chn].s3m_fine_pitch_slide = ((extcommand & 0xF) << 2);
        }
     else if (extcommand == 0)
        {
        ci[chn].s3m_pitch_slide = ABS(ci[chn].s3m_pitch_slide);
        ci[chn].s3m_fine_pitch_slide = ABS(ci[chn].s3m_fine_pitch_slide);
     }

     ci[chn].s3m_pitch_slide_on = TRUE;


}END_OF_FUNCTION (parse_s3m_portamento_down)

void do_s3m_portamento (int chn)
{
    if (mi.tick == 0)
        {
        ci[chn].period += ci[chn].s3m_fine_pitch_slide;
        ci[chn].temp_period += ci[chn].s3m_fine_pitch_slide;            
        }
    else
        {
        ci[chn].period += ci[chn].s3m_pitch_slide;
        ci[chn].temp_period += ci[chn].s3m_pitch_slide;
        }

}END_OF_FUNCTION (do_s3m_portamento)

void parse_s3m_arpeggio (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    ci[chn].arpeggio_on = TRUE;
    if (extcommand)
        ci[chn].arpeggio = extcommand;

}END_OF_FUNCTION (parse_s3m_arpeggio)


void parse_tremor (int chn, int extcommand)
{
    int on, off;

    on  = (extcommand >> 4) + 1;
    off = (extcommand & 0xF) + 1;

    ci[chn].tremor_set = (on << 8) + off;
    ci[chn].tremor_on = TRUE;

}END_OF_FUNCTION (parse_tremor)


void do_tremor (int chn)
{
    int on;
    int off;

    on  = (ci[chn].tremor_set >> 8) & 31;
    off = ci[chn].tremor_set & 31;

    ci[chn].tremor_count %= (on+off);

    if (ci[chn].tremor_count < on)
        ci[chn].temp_volume = ci[chn].volume;
    else
        ci[chn].temp_volume = 0;
        
    ci[chn].tremor_count++;

}END_OF_FUNCTION (do_tremor)

void do_global_volume (int extcommand)
{
    mi.global_volume = extcommand;

    if (mi.global_volume > 64)
        mi.global_volume = 64;

}END_OF_FUNCTION (do_global_volume)

void parse_s3m_panning (int chn, int extcommand)
{
    if (extcommand == 0)
        ci[chn].pan = 0;
    else if (extcommand <= 0x80)
        ci[chn].pan = (extcommand << 1) - 1;

}END_OF_FUNCTION (parse_s3m_panning)

void parse_s3m_retrig (int chn, int extcommand)
{
    if (extcommand)
        {
        ci[chn].s3m_retrig = extcommand & 0xF;
        ci[chn].s3m_retrig_slide = ((extcommand & 0xF0) >> 4);
        }

    ci[chn].s3m_retrig_on = TRUE;

}END_OF_FUNCTION (parse_s3m_retrig)

void do_s3m_retrig (int chn)
{
    if (ci[chn].s3m_retrig > 0)
        {
        if ( (mi.tick % ci[chn].s3m_retrig) == 0)
            {
            ci[chn].kick = TRUE;
                
            switch (ci[chn].s3m_retrig_slide)
                {
                case 1 :
                case 2 :
                case 3 :
                case 4 :
                case 5 :
                    {
                    ci[chn].temp_volume -=  ( 1 << (ci[chn].s3m_retrig_slide -1) );
                    ci[chn].volume -=  ( 1 << (ci[chn].s3m_retrig_slide -1) );
                    break;
                    }
                case 6 :
                    {
                    ci[chn].temp_volume  = (2 * ci[chn].temp_volume) / 3;
                    ci[chn].volume  = (2 * ci[chn].volume) / 3;
                    break;
                    }
                case 7 :
                    {
                    ci[chn].temp_volume =  (ci[chn].temp_volume >> 1);
                    ci[chn].volume =  (ci[chn].volume >> 1);
                    break;
                    }
                case 9 :
                case 0xA :
                case 0xB :
                case 0xC :
                case 0xD :
                    {
                    ci[chn].temp_volume += (1 << (ci[chn].s3m_retrig_slide - 9));
                    ci[chn].volume += (1 << (ci[chn].s3m_retrig_slide - 9));
                    break;
                    }
                case 0xE :
                    {
                    ci[chn].temp_volume = (3 * ci[chn].temp_volume) / 2;
                    ci[chn].volume = (3 * ci[chn].volume) / 2;
                    break;
                    }
                case 0xF :
                    {
                    ci[chn].temp_volume = ci[chn].temp_volume << 1;
                    ci[chn].volume = ci[chn].volume << 1;
                    break;
                    }

                }
            }
        }


}END_OF_FUNCTION (do_s3m_retrig)

void parse_volume_command (int chn, int volume, int note)
{
    if ( (volume >= 0x10) && (volume <= 0x50) )         // set volume
        ci[chn].volume = volume - 0x10;

    else if ( (volume >= 0x60) && (volume <= 0x6F) )    // volume slide down
        ci[chn].pro_volume_slide = -(volume & 0xF);

    else if ( (volume >= 0x70) && (volume <= 0x7F) )    // volume slide up
        ci[chn].pro_volume_slide = (volume & 0xF);

    else if ( (volume >= 0x80) && (volume <= 0x8F) )    // fine vol slide down
        ci[chn].volume -=  (volume & 0xF);

    else if ( (volume >= 0x90) && (volume <= 0x9F) )    // fine vol slide up
        ci[chn].volume +=  (volume & 0xF);

    else if ( (volume >= 0xA0) && (volume <= 0xAF) )    // vibrato speed
        ci[chn].vibrato_speed = ((volume & 0xF) << 2);

    else if ( (volume >= 0xB0) && (volume <= 0xBF) )    // do vibrato
        parse_vibrato (chn, (volume & 0xF), 5);
        
    else if ( (volume >= 0xC0) && (volume <= 0xCF) )    // set panning
        ci[chn].pan = (volume & 0xF) * 17;

    else if ( (volume >= 0xD0) && (volume <= 0xDF) )    // panning slide left
        {
        if (volume & 0xF)
            ci[chn].pan_slide_left = -(volume & 0xF);

        ci[chn].pan_slide_common = ci[chn].pan_slide_left;
        }

    else if ( (volume >= 0xE0) && (volume <= 0xEF) )    // panning slide right
        {
        if (volume & 0xF)
            ci[chn].pan_slide_right = (volume & 0xF);

        ci[chn].pan_slide_common = ci[chn].pan_slide_right;
        }
    else if ( (volume >= 0xF0) && (volume <= 0xFF) )    // porta to note
        parse_slide2period (chn, (volume & 0xF) << 4, (note > 0) ? note+ci[chn].transpose : 0); 


    if (ci[chn].volume > 64)
        ci[chn].volume = 64;
    else if (ci[chn].volume < 0)
        ci[chn].volume = 0;

}END_OF_FUNCTION (parse_volume_command)

void parse_note_command (int chn, int note)
{
    if (note == -1)
        ci[chn].volume = 0;

    if (note == -2)
        {
        ci[chn].keyon = TRUE;
        if ( (ci[chn].volenv.flg & ENV_ON) == 0)
            ci[chn].volume = 0;            
        }
	
    if (note == -3)
        {
        ci[chn].keyon = FALSE;
        }

}END_OF_FUNCTION (parse_note_command)


void lock_jgmod_player3(void)
{
    LOCK_FUNCTION(parse_volume_command);
    LOCK_FUNCTION(parse_note_command);

    LOCK_FUNCTION(parse_s3m_volume_slide);
    LOCK_FUNCTION(parse_s3m_portamento_down);
    LOCK_FUNCTION(parse_s3m_portamento_up);
    LOCK_FUNCTION(parse_s3m_arpeggio);
    LOCK_FUNCTION(parse_tremor);
    LOCK_FUNCTION(parse_s3m_panning);
    LOCK_FUNCTION(parse_s3m_retrig);

    LOCK_FUNCTION(do_s3m_volume_slide);
    LOCK_FUNCTION(do_s3m_portamento);
    LOCK_FUNCTION(do_s3m_set_tempo);
    LOCK_FUNCTION(do_s3m_set_bpm);
    LOCK_FUNCTION(do_tremor);
    LOCK_FUNCTION(do_global_volume);
    LOCK_FUNCTION(do_s3m_retrig);
}
