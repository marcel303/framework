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
 *  New effects for IT are located here. */

#include "jgmod.h"
#include "jshare.h"

namespace jgmod
{
	void convert_it_pitch (int *pitch);
}

void JGMOD_PLAYER::parse_it_note (int chn, int key, int note, int sample_no)
{
#if 1
    // todo : map key to note and sample
    // todo : start envelopes if used
    SAMPLE_INFO *si;

	if (of->flag & JGMOD_MODE_IT_INST)
	{
		// when using instruments, sample_no is actually instrument_no and we need to map key to note and sample number
		
		const INSTRUMENT_INFO * ii = of->ii + sample_no;
		
		sample_no = ii->sample_number[key];
		note = ii->key_to_note[key];
		
		sample_no--;
		jgmod::convert_it_pitch(&note);
	}
	
    if (note > 0 && sample_no >= 0)
        {
        si = of->si+sample_no;
        ci[chn].sample = sample_no;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        ci[chn].period = note2period (note, ci[chn].c2spd);
        ci[chn].kick = true;
        }
    else if ( (note > 0) && (sample_no < 0) )  // only pitch specified
        {
        si = of->si + ci[chn].sample;
        ci[chn].period = note2period (note, ci[chn].c2spd);
        ci[chn].kick = true;
        }
    else if ( (note <= 0) && (sample_no >= 0) ) // only sample_spedified
        {
        if ( (ci[chn].sample != sample_no) && (ci[chn].period > 0))
            ci[chn].kick = true;

        si = of->si + sample_no;
        ci[chn].sample = sample_no;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        }
#else
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
#endif
}
