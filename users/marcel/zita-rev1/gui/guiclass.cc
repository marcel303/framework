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

#include "guiclass.h"

#include "framework.h"

#include <math.h>

Rlinctl::Rlinctl(
	CtlCallback     * callback,
	const RotaryImg * image,
	const int         xp,
	const int         yp,
	const int         cm,
	const int         dd,
	const float       vmin,
	const float       vmax,
	const float       vini,
	const int         cbind)
	: RotaryCtl(
		callback,
		image,
		xp,
		yp,
		cbind)
	, _cm(cm)
	, _dd(dd)
	, _vmin(vmin)
	, _vmax(vmax)
	, _form(0)
{
    _count = -1;
    
    set_value(vini);
}

void Rlinctl::get_string(char * p, const int n)
{
    if (_form)
		snprintf(p, n, _form, _value);
    else
		*p = 0;
}

void Rlinctl::set_value(const float v)
{
    set_count(int(floorf(_cm * (v - _vmin) / (_vmax - _vmin) + .5f)));
}

int Rlinctl::handle_button()
{
    return PRESS;
}

int Rlinctl::handle_motion(const int dx, const int dy)
{
    return set_count(_rcount + dx - dy);
}

int Rlinctl::handle_mwheel(int dw)
{
    if (!keyboard.isDown(SDLK_LSHIFT))
		dw *= _dd;
		
    return set_count(_count + dw);
}

int Rlinctl::set_count(int u)
{
    if (u <   0) u =   0;
    if (u > _cm) u = _cm;
    
    if (u != _count)
    {
        _count = u;
        
        _value = _vmin + u * (_vmax - _vmin) / _cm;
        _angle = 270.f * (float(u) / _cm - .5f);
        
        return DELTA;
    }
    else
    {
		return 0;
	}
}

Rlogctl::Rlogctl(
	CtlCallback     * callback,
	const RotaryImg * image,
	const int         xp,
	const int         yp,
	const int         cm,
	const int         dd,
	const float       vmin,
	const float       vmax,
	const float       vini,
	const int         cbind)
	: RotaryCtl(
		callback,
		image,
		xp,
		yp,
		cbind)
	, _cm(cm)
	, _dd(dd)
	, _vmin(logf(vmin))
    , _vmax(logf(vmax))
	, _form(0)
{
    _count = -1;
    
    set_value(vini);
}

void Rlogctl::get_string(char * p, const int n)
{
    if (_form)
		snprintf (p, n, _form, _value);
    else
		*p = 0;
}

void Rlogctl::set_value(const float v)
{
    set_count(int(floorf(_cm * (logf(v) - _vmin) / (_vmax - _vmin) + .5f)));
}

int Rlogctl::handle_button()
{
    return PRESS;
}

int Rlogctl::handle_motion(const int dx, const int dy)
{
    return set_count(_rcount + dx - dy);
}

int Rlogctl::handle_mwheel(int dw)
{
	if (!keyboard.isDown(SDLK_LSHIFT))
		dw *= _dd;
		
    return set_count(_count + dw);
}

int Rlogctl::set_count(int u)
{
    if (u <   0) u =   0;
    if (u > _cm) u = _cm;
    
    if (u != _count)
    {
        _count = u;
        
        _value = expf(_vmin + u * (_vmax - _vmin) / _cm);
        _angle = 270.f * (float(u) / _cm - .5f);
        
        return DELTA;
    }
    else
    {
		return 0;
	}
}
