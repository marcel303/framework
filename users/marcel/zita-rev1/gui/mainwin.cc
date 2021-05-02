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

#include "mainwin.h"
#include "styles.h"

#include "zita-rev1.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

Mainwin::Mainwin (
	ZitaRev1::Reverb *reverb,
	bool force_ambis)
    : _stop(false)
    , _reverb(reverb)
    , _dirty(false)
{
    char        s[256];
    int         i, x;

	_ambis = force_ambis;
	
    x = 0;
    _rotary[R_DELAY] = new Rlinctl(this, &r_delay_img, x, 0, 160, 5,  0.02,  0.100,  0.04, R_DELAY);
    _rotary[R_XOVER] = new Rlogctl(this, &r_xover_img, x, 0, 200, 5,  50.0, 1000.0, 200.0, R_XOVER);
    _rotary[R_RTLOW] = new Rlogctl(this, &r_rtlow_img, x, 0, 200, 5,   1.0,    8.0,   3.0, R_RTLOW);
    _rotary[R_RTMID] = new Rlogctl(this, &r_rtmid_img, x, 0, 200, 5,   1.0,    8.0,   2.0, R_RTMID);
    _rotary[R_FDAMP] = new Rlogctl(this, &r_fdamp_img, x, 0, 200, 5, 1.5e3, 24.0e3, 6.0e3, R_FDAMP);
    x += 315;
    _rotary[R_EQ1FR] = new Rlogctl(this, &r_parfr_img, x, 0, 180, 5,  40.0,  2.5e3, 160.0, R_EQ1FR);
    _rotary[R_EQ1GN] = new Rlinctl(this, &r_pargn_img, x, 0, 150, 5, -15.0,   15.0,   0.0, R_EQ1GN);
    x += 110;
    _rotary[R_EQ2FR] = new Rlogctl(this, &r_parfr_img, x, 0, 180, 5, 160.0,   10e3, 2.5e3, R_EQ2FR);
    _rotary[R_EQ2GN] = new Rlinctl(this, &r_pargn_img, x, 0, 150, 5, -15.0,   15.0,   0.0, R_EQ2GN);
    x += 110;
    _rotary[R_OPMIX] = new Rlinctl(this, &r_opmix_img, x, 0, 180, 5,   0.0 ,   1.0,   0.5, R_OPMIX);
    _rotary[R_RGXYZ] = new Rlinctl(this, &r_rgxyz_img, x, 0, 180, 5,  -9.0 ,   9.0,   0.0, R_RGXYZ);
    
    for (i = 0; i < R_OPMIX; i++)
		_rotary[i]->show();
		
    if (_ambis) _rotary[R_RGXYZ]->show();
    else        _rotary[R_OPMIX]->show();

    _reverb->set_delay(_rotary[R_DELAY]->value());
    _reverb->set_xover(_rotary[R_XOVER]->value());
    _reverb->set_rtlow(_rotary[R_RTLOW]->value());
    _reverb->set_rtmid(_rotary[R_RTMID]->value());
    _reverb->set_fdamp(_rotary[R_FDAMP]->value());
    _reverb->set_opmix(_rotary[R_OPMIX]->value());
    _reverb->set_rgxyz(_rotary[R_RGXYZ]->value());
    
    _reverb->set_eq1(_rotary[R_EQ1FR]->value(), _rotary[R_EQ1GN]->value());
    _reverb->set_eq2(_rotary[R_EQ2FR]->value(), _rotary[R_EQ2GN]->value());
}

Mainwin::~Mainwin()
{
}

void Mainwin::tick()
{
	for (int i = 0; i < NROTARY; ++i)
		_rotary[i]->tick();
}

static void drawImage(GxTextureId image, int x, int y, int sx, int sy)
{
	setColor(colorWhite);
	gxSetTexture(image);
	drawRect(x, y, x + sx, y + sy);
	gxSetTexture(0);
}

void Mainwin::draw() const
{
    int x = 0;

    drawImage(revsect_img, x, 0, 310, 75); x += 315;
    drawImage(eq1sect_img, x, 0, 110, 75); x += 110;
    drawImage(eq2sect_img, x, 0, 110, 75); x += 110;
    
    if (_ambis) drawImage(ambsect_img, x, 0, 70, 75);
    else        drawImage(mixsect_img, x, 0, 70, 75);
    x += 70;
    
    drawImage(redzita_img, x, 0, 35, 75);
    
	for (int i = 0; i < NROTARY; ++i)
		_rotary[i]->draw();
}


void Mainwin::handle_callb(int type, Ctl *W)
{
    RotaryCtl  *R;
    int         k;

    switch (type)
    {
    case RotaryCtl::PRESS:
        R = (RotaryCtl*)W;
        k = R->cbind();
        switch (k)
        {
        default:
            ;
        }
        break;

    case RotaryCtl::DELTA:
        R = (RotaryCtl*)W;
        k = R->cbind();
        switch (k)
        {
        case R_DELAY:
            _reverb->set_delay(_rotary[R_DELAY]->value());
            break;
        case R_XOVER:
            _reverb->set_xover(_rotary[R_XOVER]->value());
            break;
        case R_RTLOW:
            _reverb->set_rtlow(_rotary[R_RTLOW]->value());
            break;
        case R_RTMID:
            _reverb->set_rtmid(_rotary[R_RTMID]->value());
            break;
        case R_FDAMP:
            _reverb->set_fdamp(_rotary[R_FDAMP]->value());
            break;
        case R_OPMIX:
            _reverb->set_opmix(_rotary[R_OPMIX]->value());
            break;
        case R_RGXYZ:
            _reverb->set_rgxyz(_rotary[R_RGXYZ]->value());
            break;
            
        case R_EQ1FR:
        case R_EQ1GN:
            _reverb->set_eq1(_rotary[R_EQ1FR]->value(), _rotary[R_EQ1GN]->value());
            break;
        case R_EQ2FR:
        case R_EQ2GN:
            _reverb->set_eq2(_rotary[R_EQ2FR]->value(), _rotary[R_EQ2GN]->value());
            break;
        }
        break;
    }

    if (!_dirty)
    {
        _dirty = true;
    }
}
