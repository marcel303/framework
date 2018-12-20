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
 *  The core of MOD player. */

#include "jgmod.h"
#include "jshare.h"

#include "framework-allegro2.h"

#include <stdio.h>

#define speed_ratio     mi.speed_ratio / 100
#define pitch_ratio     mi.pitch_ratio / 100

// the core of the mod player.
void JGMOD_PLAYER::mod_interrupt_proc (void * data)
{
	JGMOD_PLAYER * self = (JGMOD_PLAYER*)data;
	
	self->mod_interrupt();
}

void JGMOD_PLAYER::mod_interrupt ()
{
    int chn=0;
    int  sample_no;
    SAMPLE_INFO *si=nullptr;
    SAMPLE *s= nullptr;
    NOTE_INFO *ni=nullptr;
    PATTERN_INFO *pi;

    if (of == nullptr)             //return if not playing music
        return;

    if (mi.forbid == true)
        return;

    if (mi.pause == true)       //return if music is paused
        return;

    // prev_pattern() or next_pattern()
    if (mi.skip_pos)
        {
        mi.pos = mi.skip_pos-1;
        mi.trk = mi.skip_trk-1;
        mi.pat = *(of->pat_table + mi.trk);
        mi.tick = -1;

        mi.skip_pos = 0;
        mi.skip_trk = 0;
        mi.new_pos = 0;
        mi.new_trk = 0;

        for (chn=0; chn<mi.max_chn; chn++)
            {
            ci[chn].slide2period = 0;
            ci[chn].loop_start = 0;
            ci[chn].loop_times = 0;
            }

        // cut all the samples;
        for (chn=0; chn<mi.max_chn; chn++)
            {
            ci[chn].volume = 0;
            voiceApi->voice_set_volume (voice_table[chn], 0);
            voiceApi->voice_stop (voice_table[chn]);
            }

        if (mi.trk == 0)            // restart the song if trk 0
            {
            play (of, mi.loop);
            return;
            }
        }
        

    mi.tick++;
    if (mi.tick >= mi.tempo)
        if (mi.pattern_delay > 0)
            {
            mi.pattern_delay--;
            return;
            }

    if (mi.tick >= mi.tempo)
        {
        mi.tick = 0;

        // those darn pattern loop
        for (chn=0; chn<(of->no_chn); chn++)
            {
            if (chn >= mi.max_chn)
                continue;

            if (ci[chn].loop_on == false)
                continue;

            if (ci[chn].loop_times > 0)
                {
                mi.new_pos = ci[chn].loop_start+1;
                mi.new_trk = mi.trk+1;
                }
            }

        if (mi.new_pos)     //if pattern break or position jump
            {
            mi.pos = mi.new_pos-1;
            mi.trk = mi.new_trk-1;
            mi.pat = *(of->pat_table + mi.trk);

            mi.new_pos = 0;
            mi.new_trk = 0;
            }
        else
            {
            mi.pos++;

            pi = of->pi + mi.pat;
            if (mi.pos >= pi->no_pos)
                {
                mi.pos = 0;
                mi.trk++;
                mi.pat = *(of->pat_table + mi.trk);

                for (chn=0; chn<mi.max_chn; chn++)
                    {
                    ci[chn].loop_start = 0;
                    ci[chn].loop_times = 0;
                    }
                }
            }
        }


    if (mi.trk >= of->no_trk)       //check for end of song
        {
        for (chn=0; chn<mi.max_chn; chn++)
            voiceApi->voice_stop (voice_table[chn]);

        if (mi.loop == false)       // end the song
            {
            of = nullptr;
            mi.is_playing = false;
            return;
            }
       else
           {
           play (of, true);     // restart the song
           goto_track (of->restart_pos);
           return;
           }
        }


    if (mi.tick == 0)
        {
        for (chn=0; chn<(of->no_chn); chn++)
            {
            ci[chn].global_volume_slide_on = false;
            ni = get_note (of, mi.pat, mi.pos, chn);

            if (chn<JGMOD_MAX_VOICES)
                ci[chn].loop_on = false;

            // these are global commands. Should not be skipped
            if (ni->command == PTEFFECT_B)          //position jump
                do_position_jump (ni->extcommand);

            else if (ni->command == PTEFFECT_D)     //pattern break
                do_pattern_break (ni->extcommand);

            else if (ni->command == PTEFFECT_F)     //set tempo or bpm
                do_pro_tempo_bpm (ni->extcommand);

            else if (ni->command == S3EFFECT_A)     // S3M set tempo
                do_s3m_set_tempo (ni->extcommand);

            else if (ni->command == S3EFFECT_T)     // S3M set bpm
                do_s3m_set_bpm (ni->extcommand);

            else if (ni->command == S3EFFECT_V)     // set global volume
                do_global_volume (ni->extcommand);

            else if (ni->command == XMEFFECT_H && (chn < JGMOD_MAX_VOICES) )     // global volume slide
                parse_global_volume_slide (chn, ni->extcommand);

            // pattern loop
            else if ( (ni->command == PTEFFECT_E) && (ni->extcommand >> 4 == 6 ) && (chn < JGMOD_MAX_VOICES))
                do_pattern_loop (chn, ni->extcommand);

            // pattern delay
            else if ( (ni->command == 14) &&  (ni->extcommand >> 4 == 14 ) )
                mi.pattern_delay = mi.tempo * (ni->extcommand & 0xF);
            }


        // the following are not global commands. Can be skipped
        for (chn=0; chn<(of->no_chn); chn++)
            {
            if (chn >= mi.max_chn)
                continue;

            ni = get_note (of, mi.pat, mi.pos, chn);

            ci[chn].pan_slide_common = 0;
            ci[chn].pro_pitch_slide_on = false;
            ci[chn].s3m_volume_slide_on = false;
            ci[chn].xm_volume_slide_on = false;
            ci[chn].xm_pitch_slide_down_on = false;
            ci[chn].xm_pitch_slide_up_on = false;
            ci[chn].s3m_pitch_slide_on = false;
            ci[chn].sample_offset_on = false;
            ci[chn].slide2period_on = false;
            ci[chn].s3m_retrig_on = false;
            ci[chn].arpeggio_on = false;
            ci[chn].vibrato_on = false;
            ci[chn].tremolo_on = false;
            ci[chn].tremor_on = false;
            ci[chn].kick = false;

            ci[chn].delay_sample = 0;
            ci[chn].cut_sample = 0;
            ci[chn].pro_volume_slide = 0;
            ci[chn].retrig = 0;

            sample_no = ni->sample - 1;

            if ( (sample_no >= 0) && ((ni->command == PTEFFECT_3) || (ni->command == PTEFFECT_5)
                || (ni->command == XMEFFECT_5) || ( (ni->volume & 0xF0) == 0xF0))  )
                {
                if (mi.flag & (JGMOD_MODE_XM | JGMOD_MODE_IT))
                    {
                    SAMPLE_INFO *si;

                    si = of->si + get_jgmod_sample_no (sample_no, ci[chn].note);
                    ci[chn].volume = si->volume;
                    }
                else
                    ci[chn].volume = of->si[sample_no].volume;
                }

            if ( (ni->note > 0 || sample_no >= 0) && (ni->command != PTEFFECT_3)
                && (ni->command != PTEFFECT_5) && (ni->command != XMEFFECT_5) &&
                ( (ni->volume & 0xF0) != 0xF0 ) )
                {
                ci[chn].tremor_count = 0;
                ci[chn].slide2period = 0;

                if (ci[chn].vibrato_waveform <= 2)
                    ci[chn].vibrato_pointer = 0;

                if (ci[chn].tremolo_waveform <= 2)
                    ci[chn].tremolo_pointer = 0;

                if (mi.flag & JGMOD_MODE_XM)
                    parse_new_note (chn, ni->note, sample_no);
                else if (mi.flag & JGMOD_MODE_IT)
                    parse_it_note (chn, ni->it_note, ni->note, sample_no);
                else
                    parse_old_note (chn, ni->note, sample_no);
                }

            if (ni->note < 0)
                parse_note_command (chn, ni->note);

            if (ni->volume >= 0x10)
                parse_volume_command (chn, ni->volume, ni->note);


            if (ni->command == PTEFFECT_0)              // mod arpeggio
                parse_pro_arpeggio (chn, ni->extcommand);   

            else if (ni->command == PTEFFECT_1)         // pitch slide up
                parse_pro_pitch_slide_up (chn, ni->extcommand);

            else if (ni->command == PTEFFECT_2)         // pitch slide down
                parse_pro_pitch_slide_down (chn, ni->extcommand);

            else if (ni->command == PTEFFECT_3)         // slide to note
                parse_slide2period (chn, ni->extcommand, (ni->note > 0) ? ni->note+ci[chn].transpose : 0);
                                                                  
            else if (ni->command == PTEFFECT_4)         // vibrato
                parse_vibrato (chn, ni->extcommand, 5);

            else if (ni->command == PTEFFECT_5)         // slide to note + volume slide
                {
                parse_slide2period (chn, 0, ni->note);
                parse_pro_volume_slide (chn, ni->extcommand);
                }

            else if (ni->command == PTEFFECT_6)         // vibrato + volume slide
                {
                parse_vibrato (chn, 0, 5);
                parse_pro_volume_slide (chn, ni->extcommand);
                }

            else if (ni->command == PTEFFECT_7)         // tremolo
                parse_tremolo (chn, ni->extcommand, 6);

            else if (ni->command == PTEFFECT_8)         // set pan
                ci[chn].pan = ni->extcommand;

            else if (ni->command == PTEFFECT_9)         // sample offset
                {
                if ((ni->note > 0) || (ni->sample-1 >= 0))
                    {
                    ci[chn].sample_offset_on = true;
                    ci[chn].kick = true;
                    if (ni->extcommand)
                        ci[chn].sample_offset = (ni->extcommand << 8);
                    }
                }

            else if (ni->command == PTEFFECT_A)         // volume_slide
                parse_pro_volume_slide  (chn, ni->extcommand);

            else if (ni->command == PTEFFECT_C)         // set volume
                ci[chn].volume = ni->extcommand;

            else if (ni->command == PTEFFECT_E)         // extended effects
                parse_extended_command (chn, ni->extcommand);

            else if (ni->command == S3EFFECT_D)         // s3m volume slide
                parse_s3m_volume_slide (chn, ni->extcommand);

            else if (ni->command == S3EFFECT_E)         // s3m porta down
                parse_s3m_portamento_down(chn, ni->extcommand);

            else if (ni->command == S3EFFECT_F)         // s3m porta up
                parse_s3m_portamento_up (chn, ni->extcommand);

            else if (ni->command == S3EFFECT_I)         // tremor
                parse_tremor (chn, ni->extcommand);

            else if (ni->command == S3EFFECT_J)         // s3m arpeggio
                parse_s3m_arpeggio (chn, ni->extcommand);

            else if (ni->command == S3EFFECT_K)         // vibrato + vol slide
                {
                parse_vibrato (chn, 0, 5);
                parse_s3m_volume_slide (chn, ni->extcommand);
                }

            else if (ni->command == S3EFFECT_L)         // porta + vol slide
                {
                parse_slide2period (chn, 0, ni->note);
                parse_s3m_volume_slide (chn, ni->extcommand);
                }

            else if (ni->command == S3EFFECT_Q)         // multi retrig
                parse_s3m_retrig(chn, ni->extcommand);

            else if (ni->command == S3EFFECT_R)         // s3m tremolo
                parse_tremolo (chn, ni->extcommand, 7);

            else if (ni->command == S3EFFECT_U)         // fine vibrato
                parse_vibrato (chn, ni->extcommand, 7);

            else if (ni->command == S3EFFECT_X)         // s3m set panning
                parse_s3m_panning (chn, ni->extcommand);

            else if (ni->command == XMEFFECT_1)         // xm porta up
                parse_xm_pitch_slide_up (chn, ni->extcommand);

            else if (ni->command == XMEFFECT_2)         // xm porta down
                parse_xm_pitch_slide_down (chn, ni->extcommand);

            else if (ni->command == XMEFFECT_5)         // slide to note + vol slide
                {
                parse_slide2period (chn, 0, (ni->note > 0) ? ni->note+ci[chn].transpose : 0);
                parse_xm_volume_slide (chn, ni->extcommand);
                }

            else if (ni->command == XMEFFECT_6)         // vibrato + XM vol slide
                {
                parse_vibrato (chn, 0, 5);
                parse_xm_volume_slide (chn, ni->extcommand);
                }

            else if (ni->command == XMEFFECT_A)         // xm volume slide
                parse_xm_volume_slide (chn, ni->extcommand);

            else if (ni->command == XMEFFECT_P)         // xm panning slide
                parse_xm_pan_slide(chn, ni->extcommand);

            else if (ni->command == XMEFFECT_K)         // key off
                {
                ci[chn].keyon = true;
                if ( (ci[chn].volenv.flg & JGMOD_ENV_ON) == 0)
                    ci[chn].volume = 0;            
                }

            else if (ni->command == XMEFFECT_L)         // Set envelop position
                parse_xm_set_envelop_position (&ci[chn].volenv, ni->extcommand);

            else if (ni->command == XMEFFECT_X)         // extra fine porta
                do_xm_x (chn, ni->extcommand);
            

            ci[chn].temp_pan    = ci[chn].pan;
            ci[chn].temp_volume = ci[chn].volume;
            ci[chn].temp_period = ci[chn].period;
            }
        }


// -- effects updated only after tick 0 --------------------------------------
    if (mi.tick)
        {
        for (chn=0; chn < (of->no_chn); chn++)
            {
            if (chn >= mi.max_chn)
                continue;

            if (ci[chn].s3m_retrig_on == true)
                do_s3m_retrig (chn);

            if (ci[chn].arpeggio_on == true)
                do_arpeggio (chn);

            if (ci[chn].pro_pitch_slide_on == true)
                {
                ci[chn].period += ci[chn].pro_pitch_slide;
                ci[chn].temp_period += ci[chn].pro_pitch_slide;
                }

            if (ci[chn].pro_volume_slide)
                {
                ci[chn].volume += ci[chn].pro_volume_slide;
                ci[chn].temp_volume += ci[chn].pro_volume_slide;
                }

            if (ci[chn].retrig)
                {
                if ((mi.tick % ci[chn].retrig) == 0)
                    ci[chn].kick = true;
                }

            if (ci[chn].cut_sample)
                {
                if (mi.tick == ci[chn].cut_sample)
                    {
                    ci[chn].volume = 0;
                    ci[chn].temp_volume = 0;
                    }
                }
            
            if (ci[chn].slide2period_on == true)
                do_slide2period(chn);

            if (ci[chn].delay_sample)
                do_delay_sample(chn);

            if (ci[chn].tremor_on == true)
                do_tremor (chn);

            if (ci[chn].xm_volume_slide_on == true)
                do_xm_volume_slide (chn);

            if (ci[chn].xm_pitch_slide_up_on == true)
                do_xm_pitch_slide_up (chn);

            if (ci[chn].xm_pitch_slide_down_on == true)
                do_xm_pitch_slide_down (chn);

            if (ci[chn].pan_slide_common)
                do_xm_pan_slide (chn);

            if (ci[chn].global_volume_slide_on == true)
                do_global_volume_slide (chn);
            }
        }

// -- effects updated every tick ---------------------------------------------
    for (chn=0; chn < (of->no_chn); chn++)
        {
        if (chn >= mi.max_chn)
            continue;

        if (ci[chn].vibrato_on == true)
            do_vibrato (chn);

        if (ci[chn].tremolo_on == true)
            do_tremolo (chn);

        if (ci[chn].s3m_volume_slide_on == true)
            do_s3m_volume_slide (chn);

        if (ci[chn].s3m_pitch_slide_on == true)
            do_s3m_portamento (chn);
        }

        

//-- updates the sound output ------------------------------------------------
    for (chn=0; chn < (of->no_chn); chn++)
        {
        if (chn >= mi.max_chn)
            continue;

        if (ci[chn].delay_sample)
            continue;

        if (ci[chn].temp_period < 4)
            ci[chn].temp_period = 4;
        
        if (ci[chn].volume < 0)
            ci[chn].volume = 0;
        else if (ci[chn].volume > 64)
            ci[chn].volume = 64;

        if (ci[chn].temp_volume < 0)
            ci[chn].temp_volume = 0;
        else if (ci[chn].temp_volume > 64)
            ci[chn].temp_volume = 64;

        if (ci[chn].pan < 0)
            ci[chn].pan = 0;
        else if (ci[chn].pan > 255)
            ci[chn].pan = 255;

        if (ci[chn].temp_pan < 0)
            ci[chn].temp_pan = 0;
        else if (ci[chn].temp_pan > 255)
            ci[chn].temp_pan = 255;

        if (ci[chn].kick == true)       // start a sample
            {
            s = of->s + ci[chn].sample;
            si= of->si + ci[chn].sample;

            voiceApi->reallocate_voice (voice_table[chn], s);

            if (si->loop & JGMOD_LOOP_ON)
                voiceApi->voice_set_playmode (voice_table[chn], PLAYMODE_LOOP);

            if (si->loop & JGMOD_LOOP_BIDI)
                voiceApi->voice_set_playmode (voice_table[chn], PLAYMODE_BIDIR|PLAYMODE_LOOP);

            //voice_start(voice_table[chn]);
            //ci[chn].kick = FALSE;
            }

        if (ci[chn].sample_offset_on == true)
            {
            voiceApi->voice_set_position (voice_table[chn], ci[chn].sample_offset);
            ci[chn].sample_offset_on = false;
            }

        if (ci[chn].temp_period > 0)
            voiceApi->voice_set_frequency (voice_table[chn], period2pitch (ci[chn].temp_period) * pitch_ratio);

        process_envelope (&ci[chn].volenv, 64, ci[chn].keyon);
        process_envelope (&ci[chn].panenv, 32, ci[chn].keyon);
        voiceApi->voice_set_volume (voice_table[chn], calc_volume (chn));
        voiceApi->voice_set_pan (voice_table[chn], calc_pan (chn));

        if (ci[chn].keyon == true)
            {
            ci[chn].volfade -= ci[chn].instfade;
            if (ci[chn].volfade < 0)
                ci[chn].volfade = 0;
            }

        if (ci[chn].kick == true)
            {
            voiceApi->voice_start(voice_table[chn]);
            ci[chn].kick = false;
            }
        }
}
