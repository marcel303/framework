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

#include "rotary.h"

#include <math.h>

RotaryCtl::RotaryCtl(
	CtlCallback * callback,
	const RotaryImg * image,
	const int xp,
	const int yp,
	const int cbind)
	: Ctl(
		image->_x0 + xp,
		image->_y0 + yp,
		callback)
	, _cbind(cbind)
	, _image(image)
	, _count(0)
	, _value(0)
	, _angle(0)
	, _rcount(0)
	, _button(0)
{
} 

void RotaryCtl::tick()
{
	if (!visible)
		return;
		
	if (mouse.wentDown(BUTTON_LEFT))
	{
		const float d = hypotf(
			mouse.x - (_xp + _image->_xref),
			mouse.y - (_yp + _image->_yref));
		
		if (d <= _image->_rad + 3)
		{
			_rx = mouse.x;
			_ry = mouse.y;
			_rcount = _count;
			_button = 1;
			
			const int r = handle_button();
			
			if (r != 0)
			{
				callback(r);
			}
		}
	}
	else if (_button != 0 && mouse.wentUp(BUTTON_LEFT))
    {
        _button = 0;
        
        callback(RELSE);
    }
    
    if (_button != 0)
    {
		const int dx = mouse.x - _rx;
        const int dy = mouse.y - _ry;
        
        const int r = handle_motion(dx, dy);
        
        if (r != 0)
        {
            callback(r);
        }
    }
}

void RotaryCtl::draw()
{
	if (!visible)
		return;
		
	gxPushMatrix();
	{
		gxTranslatef(_xp, _yp, 0);

		const float a = _angle * float(M_PI) / 180.f;
		const float r = _image->_rad;
		const float x = _image->_xref;
		const float y = _image->_yref;
		
		hqBegin(HQ_LINES);
		{
			setColor(colorBlack);
			hqLine(
				x,
				y,
				1.8f,
				x + r * sinf(a),
				y - r * cosf(a),
				1.8f);
		}
		hqEnd();
    }
    gxPopMatrix();
}
