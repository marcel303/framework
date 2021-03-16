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

#include "guiclass.h"
#include "jclient.h"

struct Mainwin : CtlCallback
{
    enum
	{
		XSIZE = 640,
		YSIZE = 75
	};

    Mainwin(
		AudioStream_Reverb *jclient,
		bool force_ambis = false);
    ~Mainwin();
    
    Mainwin(const Mainwin&) = delete;
    Mainwin& operator=(const Mainwin&) = delete;
	
	void tick();
	void draw() const;

private:

    enum
    {
		R_DELAY,
		R_XOVER,
		R_RTLOW,
		R_RTMID,
		R_FDAMP,
		R_EQ1FR,
		R_EQ1GN,
		R_EQ2FR,
		R_EQ2GN,
		R_OPMIX,
		R_RGXYZ,
		NROTARY
	};

    virtual void handle_callb(int type, Ctl *ctl) override;

    bool                _stop;
    bool                _ambis;
    AudioStream_Reverb *_jclient;
    RotaryCtl          *_rotary [NROTARY];
    bool                _dirty;
};
