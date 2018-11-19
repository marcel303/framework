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
#include <assert.h>

namespace jgmod
{
	void convert_it_pitch (int *pitch);
}

void JGMOD_PLAYER::parse_it_note (int chn, int key, int note, int sample_no)
{
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
	
	if (of->flag & JGMOD_MODE_IT_INST)
	{
		const int instrument_no = sample_no;
		
		assert(instrument_no >= 0 && instrument_no < of->no_instrument);
		if (instrument_no >= 0 && instrument_no < of->no_instrument)
		{
			const INSTRUMENT_INFO * ii = of->ii + instrument_no;
			
			// when using instruments, sample_no is actually instrument_no and we need to map key to note and sample number
			
			sample_no = ii->sample_number[key];
			sample_no--;
			
			if (ii->key_to_note[key] != 0)
			{
				note = ii->key_to_note[key];
				jgmod::convert_it_pitch(&note);
			}
			
			assert(note >= 0);
			
			// set instrument fadeout
			ci[chn].instfade = ii->volume_fadeout;

			// start the volume envelope
			start_envelope(
				&ci[chn].volenv, ii->volenv, ii->volpos, ii->vol_type,
				ii->no_volenv, ii->vol_begin, ii->vol_end, ii->vol_susbeg, ii->vol_susend);

			// start the panning envelope
			start_envelope(
				&ci[chn].panenv, ii->panenv, ii->panpos, ii->pan_type,
				ii->no_panenv, ii->pan_begin, ii->pan_end, ii->pan_susbeg, ii->pan_susend);
		}
	}
	
    if (note > 0 && sample_no >= 0)
	{
        const SAMPLE_INFO *si = of->si+sample_no;
		
        ci[chn].sample = sample_no;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        ci[chn].period = note2period (note, ci[chn].c2spd);
        ci[chn].kick = true;
        ci[chn].pan = si->pan;
	}
    else if ( (note > 0) && (sample_no < 0) )  // only pitch specified
	{
        ci[chn].period = note2period (note, ci[chn].c2spd);
        ci[chn].kick = true;
	}
    else if ( (note <= 0) && (sample_no >= 0) ) // only sample_spedified
	{
        if ( (ci[chn].sample != sample_no) && (ci[chn].period > 0))
            ci[chn].kick = true;

        const SAMPLE_INFO *si = of->si + sample_no;
        ci[chn].sample = sample_no;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        ci[chn].pan = si->pan;
	}
}
