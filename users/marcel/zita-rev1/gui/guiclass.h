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

#include "rotary.h"

struct Rlinctl : RotaryCtl
{
    Rlinctl(
		CtlCallback     * callback,
		const RotaryImg * image,
		const int         xp,
		const int         yp,
		const int         cm,
		const int         dd,
		const float       vmin,
		const float       vmax,
		const float       vini,
		const int         cbind = 0);

    virtual void set_value(const float v) override;
    virtual void get_string(char * p, const int n) override;

private:

    virtual int handle_button() override;
    virtual int handle_motion(const int dx, const int dy) override;
    virtual int handle_mwheel(const int dw) override;
    
    int set_count(const int u);

    const int    _cm;
    const int    _dd;
    const float  _vmin;
    const float  _vmax;
    const char * _form;
};

struct Rlogctl : RotaryCtl
{
    Rlogctl(
		CtlCallback     * callback,
		const RotaryImg * image,
		const int         xp,
		const int         yp,
		const int         cm,
		const int         dd,
		const float       vmin,
		const float       vmax,
		const float       vini,
		const int         cbind = 0);

    virtual void set_value(const float v) override;
    virtual void get_string(char * p, const int n) override;

private:

    virtual int handle_button() override;
    virtual int handle_motion(const int dx, const int dy) override;
    virtual int handle_mwheel(const int dw) override;
    
    int set_count(const int u);

    const int     _cm;
    const int     _dd;
    const float   _vmin;
    const float   _vmax;
    const char  * _form;
};
