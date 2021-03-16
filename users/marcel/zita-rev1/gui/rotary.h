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

#include "framework.h"

struct Ctl;
struct CtlCallback;

struct CtlCallback
{
	virtual void handle_callb(const int type, Ctl * ctl) = 0;
};

struct Ctl
{
	Ctl(const int xp, const int yp, CtlCallback * in_callback)
		: _xp(xp)
		, _yp(yp)
		, _callback(in_callback)
		, visible(false)
	{
	}
	
	virtual ~Ctl()
	{
	}
	
	const int _xp;
	const int _yp;
	CtlCallback * const _callback;
	
	bool visible;
	
	void callback(const int type)
	{
		_callback->handle_callb(type, this);
	}
	
	void show()
	{
		visible = true;
	}
};

//

struct RotaryImg
{
    GxTextureId _image[4];
    int         _x0;
    int         _y0;
    int         _dx;
    int         _dy;
    float       _xref;
    float       _yref;
    float       _rad;
};

struct RotaryCtl : Ctl
{
    RotaryCtl(
		CtlCallback * callback,
		const RotaryImg * image,
		const int xp,
		const int yp,
		const int cbind = 0);

    enum
	{
		NOP = 200,
		PRESS,
		RELSE,
		DELTA
	};

    int   cbind() const { return _cbind; }
    float value() const { return _value; }

    virtual void set_value(const float v) = 0;
    virtual void get_string(char * p, const int n) { }

	CtlCallback     * _callback;
    const int         _cbind;
    const RotaryImg * _image;
    
    int          _count;
    float        _value;
    float        _angle;

	int          _rcount;
    int          _rx, _ry;
    int          _button;
    
    void tick();
    void draw();

    virtual int handle_button() = 0;
    virtual int handle_motion(const int dx, const int dy) = 0;
    virtual int handle_mwheel(const int dw) = 0;
};
