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

#pragma once

#include "audiostream/AudioStream.h"
#include "audiostream/AudioStreamWave.h"

#include "reverb.h"

struct AudioStream_Reverb : AudioStream
{
    AudioStream_Reverb(
		const int sampleRate,
		const bool ambis);

	virtual int Provide(int numSamples, AudioSample * samples) override;

	ZitaRev1::Reverb * reverb() { return &_reverb; };
	
	AudioStreamWave  _source;
	
    unsigned int     _fsamp;
    bool             _ambis;
    int              _fragm;
    int              _nsamp;
    ZitaRev1::Reverb _reverb;
};
