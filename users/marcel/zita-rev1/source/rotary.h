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

struct CtlCallback
{
	virtual void handle_callb(int type, Ctl *ctl) = 0;
};

struct Ctl
{
	Ctl(int xp, int yp, CtlCallback * in_callback)
		: _xp(xp)
		, _yp(yp)
		, _callback(in_callback)
		, visible(false)
	{
	}
	
	int _xp;
	int _yp;
	CtlCallback * _callback;
	bool visible;
	
	void callback(int type)
	{
		_callback->handle_callb(type, this);
	}
	
	void show()
	{
		visible = true;
	}
};

struct RotaryImg
{
    GxTextureId _image [4];
    char        _lncol [4];
    int         _x0;
    int         _y0;
    int         _dx;
    int         _dy;
    double      _xref;
    double      _yref;
    double      _rad;
};

struct RotaryCtl : Ctl
{
    RotaryCtl(
		CtlCallback *callback,
		RotaryImg *image,
		int xp,
		int yp,
		int cbind = 0);

    virtual ~RotaryCtl() { }

    enum
	{
		NOP = 200,
		PRESS,
		RELSE,
		DELTA
	};

    int    cbind() { return _cbind; }
    int    state() { return _state; }
    double value() { return _value; }

    virtual void set_state(int s);
    virtual void set_value(double v) = 0;
    virtual void get_string(char *p, int n) { }

	CtlCallback *_callback;
    int          _cbind;
    RotaryImg   *_image;
    int          _state;
    int          _count;
    int          _range;
    double       _value;
    double       _angle;

	int          _rcount;
    int          _button;
    int          _rx, _ry;
    
    void tick();
    void draw();

    virtual int handle_button() = 0;
    virtual int handle_motion(int dx, int dy) = 0;
    virtual int handle_mwheel(int dw) = 0;
};
