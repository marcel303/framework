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
 *  New effects for XM are located here. */

#include "jgmod.h"
#include "jshare.h"

#include "framework-allegro2.h"

void JGMOD_PLAYER::parse_new_note (int chn, int note, int sample_no)
{
    SAMPLE_INFO *si;
    INSTRUMENT_INFO *ii = nullptr;


    ci[chn].keyon = false;
    ci[chn].volfade = 32768;
    ci[chn].instfade = 0;
    
    ci[chn].volenv.flg = 0;
    ci[chn].volenv.pts = 0;
    ci[chn].volenv.a = 0;
    ci[chn].volenv.b = 0;
    ci[chn].volenv.p = 0;
    ci[chn].volenv.v = 64;

    ci[chn].panenv.flg = 0;
    ci[chn].panenv.pts = 0;
    ci[chn].panenv.a = 0;
    ci[chn].panenv.b = 0;
    ci[chn].panenv.p = 0;
    ci[chn].panenv.v = 32;

    note--;
    if (note >= 0 && sample_no >= 0)            // pitch and note specified
        {
        ci[chn].note = note;
        ci[chn].instrument = sample_no;
        ci[chn].sample = get_jgmod_sample_no (sample_no, ci[chn].note);

        si = of->si+ci[chn].sample;

        ci[chn].volume = si->volume;
        ci[chn].transpose = si->transpose;
        ci[chn].c2spd = si->c2spd;
        ci[chn].period = note2period (ci[chn].note+ci[chn].transpose, ci[chn].c2spd);
        ci[chn].kick = true;
        ci[chn].pan = si->pan;
        }
    else if ( (note >= 0) && (sample_no < 0) )  // only note specified
        {
        ci[chn].note = note;
        ci[chn].sample = get_jgmod_sample_no (ci[chn].instrument, note);

        si = of->si + ci[chn].sample;

        ci[chn].transpose = si->transpose;
        ci[chn].c2spd = si->c2spd;
        ci[chn].period = note2period (ci[chn].note+ci[chn].transpose, ci[chn].c2spd);
        ci[chn].kick = true;
        }
    /*
    else if ( (note < 0) && (sample_no >= 0) ) // only sample_spedified
        {
        if ( (ci[chn].instrument != sample_no) && (ci[chn].period > 0))
            ci[chn].kick = TRUE;

        ci[chn].instrument = sample_no;
        ci[chn].sample = get_jgmod_sample_no (sample_no, ci[chn].note);

        si = of->si + ci[chn].sample;

        ci[chn].transpose = si->transpose;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        ci[chn].pan = si->pan;
        }   */


    // make sure it doesn't access invalid instruments with could happen
    if ( (ci[chn].instrument < of->no_instrument) && (ci[chn].instrument >= 0) )
        {
        ii = of->ii+ci[chn].instrument;
        ci[chn].instfade = ii->volume_fadeout;

        // initialize the volume envelope
        start_envelope (&ci[chn].volenv, ii->volenv, ii->volpos, ii->vol_type,
            ii->no_volenv, ii->vol_begin, ii->vol_end, ii->vol_susbeg, ii->vol_susend);

        // initialize the pan envelope                                            
        start_envelope (&ci[chn].panenv, ii->panenv, ii->panpos, ii->pan_type,
            ii->no_panenv, ii->pan_begin, ii->pan_end, ii->pan_susbeg, ii->pan_susend);
        }

}

void JGMOD_PLAYER::parse_xm_volume_slide (int chn, int extcommand)
{
    ci[chn].xm_volume_slide_on = true;

    if (extcommand)
        {
        if (extcommand & 0xF0)
            ci[chn].xm_volume_slide = ((extcommand & 0xF0) >> 4);
        else
            ci[chn].xm_volume_slide = -(extcommand & 0xF);
        }

}

void JGMOD_PLAYER::do_xm_volume_slide (int chn)
{
    ci[chn].volume += ci[chn].xm_volume_slide;
    ci[chn].temp_volume += ci[chn].xm_volume_slide;

}

void JGMOD_PLAYER::parse_xm_pitch_slide_up (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    ci[chn].xm_pitch_slide_up_on = true;
    if (extcommand)
        ci[chn].xm_pitch_slide_up = -(extcommand << 2);

}

void JGMOD_PLAYER::parse_xm_pitch_slide_down (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    ci[chn].xm_pitch_slide_down_on = true;
    if (extcommand)
        ci[chn].xm_pitch_slide_down = (extcommand << 2);

}

void JGMOD_PLAYER::do_xm_pitch_slide_down (int chn)
{
    ci[chn].period += ci[chn].xm_pitch_slide_down;
    ci[chn].temp_period += ci[chn].xm_pitch_slide_down;

}

void JGMOD_PLAYER::do_xm_pitch_slide_up (int chn)
{
    ci[chn].period += ci[chn].xm_pitch_slide_up;
    ci[chn].temp_period += ci[chn].xm_pitch_slide_up;

    if (ci[chn].period < 40)
        ci[chn].period = 40;

    if (ci[chn].temp_period < 40)
        ci[chn].temp_period = 40;

}

void JGMOD_PLAYER::do_xm_pan_slide (int chn)
{
    ci[chn].pan += ci[chn].pan_slide_common;
    ci[chn].temp_pan += ci[chn].pan_slide_common;

}

void JGMOD_PLAYER::parse_xm_pan_slide (int chn, int extcommand)
{
    if (extcommand)
        {
        if (extcommand & 0xF0)
            ci[chn].pan_slide = (extcommand & 0xF0) >> 4;
        else if (extcommand & 0xF)
            ci[chn].pan_slide = -(extcommand & 0xF);
        }

    ci[chn].pan_slide_common = ci[chn].pan_slide;

}

void JGMOD_PLAYER::parse_global_volume_slide (int chn, int extcommand)
{
    ci[chn].global_volume_slide_on = true;

    if (extcommand)
        {
        if (extcommand & 0xF0)
            ci[chn].global_volume_slide = ((extcommand & 0xF0) >> 4);
        else if (extcommand & 0xF)
            ci[chn].global_volume_slide = -(extcommand & 0xF);
        }

}

void JGMOD_PLAYER::do_global_volume_slide (int chn)
{
    mi.global_volume += ci[chn].global_volume_slide;

    if (mi.global_volume > 64)
        mi.global_volume = 64;
    else if (mi.global_volume < 0)
        mi.global_volume = 0;

}

void JGMOD_PLAYER::do_xm_x (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    if ( ((extcommand & 0xF0) >> 4) == 1)           // extra fine porta up
        {
        if (extcommand & 0xF)
            ci[chn].xm_x_up = -(extcommand & 0xF);

        ci[chn].period += ci[chn].xm_x_up;
        }
    else if ( ((extcommand & 0xF0) >> 4) == 2)      // extra fine porta up
        {
        if (extcommand & 0xF)
            ci[chn].xm_x_down = (extcommand & 0xF);

        ci[chn].period += ci[chn].xm_x_down;
        }

}

void JGMOD_PLAYER::start_envelope (volatile ENVELOPE_INFO *t, int *env, int *pos, int flg,
    int pts, int loopbeg, int loopend, int susbeg, int susend)
{
    int temp;

    for (temp = 0; temp < JGMOD_MAX_ENVPTS; temp++)
        {
        t->env[temp] = env[temp];
        t->pos[temp] = pos[temp];
        }

    t->flg = flg;
    t->pts = pts;
    t->loopbeg = loopbeg;
    t->loopend = loopend;
    t->susbeg  = susbeg;
    t->susend  = susend;
    t->a = 0;
    t->b = 1;
    t->p = 0;

    if ( (t->flg & JGMOD_ENV_SUS) && (t->susbeg == 0) )
        t->b = 0;

    if ( (t->flg & JGMOD_ENV_ON) && (t->pts == 1) )
        t->b = 0;

}

void JGMOD_PLAYER::process_envelope (volatile ENVELOPE_INFO *t, int v, int keyon)
{
    t->v = v;

    if ( (t->flg == 0) || (t->pts == 0) )
        return;

    if ( (t->flg & JGMOD_ENV_ON) && t->pts)
        {
        int a, b, p;

        a = t->a;
        b = t->b;
        p = t->p;

        if (a == b)
            t->v = t->env[a];
        else
            t->v = interpolate(p, t->pos[a], t->pos[b], t->env[a], t->env[b]);

        p++;

        // pointer reached point b
        if (p >= t->pos[b])
            {
            a = b;
            b++;

            if ( (t->flg & JGMOD_ENV_SUS) && (keyon == false) && (b > t->susend) )
                	{
					a = t->susbeg;
					p = t->pos[a];
					if (t->susbeg == t->susend)
						b = a;
					else
						b = a + 1;
					}
            else if ( (t->flg & JGMOD_ENV_LOOP) && (b > t->loopend) )
                {
                a = t->loopbeg;
                p = t->pos[a];
                if (t->loopbeg == t->loopend)
                    b = a;
                else
                    b = a + 1;
                }
            else if (b >= t->pts)
                {
                b--;
                p--;
                }
            }

        t->p = p;
        t->a = a;
        t->b = b;
        }
    
}

void JGMOD_PLAYER::parse_xm_set_envelop_position (volatile ENVELOPE_INFO *t, int extcommand)
{
    int no_points;
    
    if (t->pts > 0)
        {
        no_points =  t->pos[t->pts-1];
        t->p = (extcommand > no_points) ? no_points : extcommand;

        t->b = 0;
        while (t->p >= t->pos[t->b] )
            (t->b)++;

        t->a = t->b - 1;
        if (t->a < 0)
            t->a = 0;
        }

}
