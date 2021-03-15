// ----------------------------------------------------------------------
//
//  Copyright (C) 2010 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// ----------------------------------------------------------------------

#include "jclient.h"

AudioStream_Reverb::AudioStream_Reverb(const int in_fsamp, const bool in_ambis)
{
	_fsamp = in_fsamp;
	_ambis = in_ambis;
	
    _fragm = 1024;
    _nsamp = 0;

    _reverb.init (_fsamp, _ambis);
}

int AudioStream_Reverb::Provide(int numSamples, AudioSample * samples)
{
    int   i, k, n_inp, n_out;

	float * inp[2] =
	{
		(float*)alloca(numSamples * sizeof(float)),
		(float*)alloca(numSamples * sizeof(float))
	};
	
	float * out[2] =
	{
		(float*)alloca(numSamples * sizeof(float)),
		(float*)alloca(numSamples * sizeof(float))
	};
	
	for (int i = 0; i < numSamples; ++i)
	{
		inp[0][i] = 0.f;
		inp[1][i] = 0.f;
	}
			
    for (int i = 0; i < 2; ++i)
    {
        if (!_nsamp)
        {
            _reverb.prepare (_fragm);
            _nsamp = _fragm;
        }
        
        k = (_nsamp < numSamples) ? _nsamp : numSamples;
        
        _reverb.process (k, inp, out);
        
	#if TZ_TODO // why is reverb processed in chunks ? to ensure prepare is called regularly ?
        for (i = 0; i < n_inp; i++) inp [i] += k;
        for (i = 0; i < n_out; i++) out [i] += k;
        frames -= k;
        _nsamp -= k;
	#endif
    }
    
    for (int i = 0; i < numSamples; ++i)
    {
		samples[i].channel[0] = 0;
		samples[i].channel[1] = 0;
	}

    return numSamples;
}
