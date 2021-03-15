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

#include "framework.h"

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
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			inp[0][i] = random<float>(-.02f, +.02f);
			inp[1][i] = random<float>(-.02f, +.02f);
		}
	}
	
	const int n_inp = 2;
	const int n_out = 2;
		
	int numFramesLeft = numSamples;
	
    while (numFramesLeft != 0)
    {
        if (!_nsamp)
        {
            _reverb.prepare (_fragm);
            _nsamp = _fragm;
        }
        
        const int numFramesThisLoop =
			_nsamp < numFramesLeft
			? _nsamp
			: numFramesLeft;
        
        _reverb.process(numFramesThisLoop, inp, out);
        
        for (int i = 0; i < n_inp; i++) inp[i] += numFramesThisLoop;
        for (int i = 0; i < n_out; i++) out[i] += numFramesThisLoop;
        
        numFramesLeft -= numFramesThisLoop;
        _nsamp        -= numFramesThisLoop;
    }
    
    for (int i = 0; i < numSamples; ++i)
    {
		samples[i].channel[0] = out[0][i] * (1 << 15);
		samples[i].channel[1] = out[1][i] * (1 << 15);
	}

    return numSamples;
}
