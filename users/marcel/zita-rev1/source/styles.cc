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

#include "styles.h"

#include "framework.h"

GxTextureId revsect_img;
GxTextureId eq1sect_img;
GxTextureId eq2sect_img;
GxTextureId mixsect_img;
GxTextureId ambsect_img;
GxTextureId redzita_img;
GxTextureId sm_img;

RotaryImg  r_delay_img;
RotaryImg  r_xover_img;
RotaryImg  r_rtlow_img;
RotaryImg  r_rtmid_img;
RotaryImg  r_fdamp_img;
RotaryImg  r_parfr_img;
RotaryImg  r_pargn_img;
RotaryImg  r_opmix_img;
RotaryImg  r_rgxyz_img;

bool styles_init()
{
    revsect_img = getTexture("revsect.png");
    eq1sect_img = getTexture("eq1sect.png");
    eq2sect_img = getTexture("eq2sect.png");
    mixsect_img = getTexture("mixsect.png");
    ambsect_img = getTexture("ambsect.png");
    redzita_img = getTexture("redzita.png");
    sm_img      = getTexture("sm.png");

    if (!revsect_img || !mixsect_img || !ambsect_img ||
		!eq1sect_img || !eq2sect_img || !redzita_img ||
		!sm_img)
    {
        logError("can't load images");
        return false;
    }

    r_delay_img._image [0] = revsect_img;
    r_delay_img._lncol [0] = 0;
    r_delay_img._x0 = 30;
    r_delay_img._y0 = 32;
    r_delay_img._dx = 23;
    r_delay_img._dy = 23;
    r_delay_img._xref = 11.5;
    r_delay_img._yref = 11.5;
    r_delay_img._rad = 11;

    r_xover_img._image [0] = revsect_img;
    r_xover_img._lncol [0] = 0;
    r_xover_img._x0 = 92;
    r_xover_img._y0 = 17;
    r_xover_img._dx = 23;
    r_xover_img._dy = 23;
    r_xover_img._xref = 11.5;
    r_xover_img._yref = 11.5;
    r_xover_img._rad = 11;

    r_rtlow_img._image [0] = revsect_img;
    r_rtlow_img._lncol [0] = 0;
    r_rtlow_img._x0 = 147;
    r_rtlow_img._y0 = 17;
    r_rtlow_img._dx = 23;
    r_rtlow_img._dy = 23;
    r_rtlow_img._xref = 11.5;
    r_rtlow_img._yref = 11.5;
    r_rtlow_img._rad = 11;

    r_rtmid_img._image [0] = revsect_img;
    r_rtmid_img._lncol [0] = 0;
    r_rtmid_img._x0 = 207;
    r_rtmid_img._y0 = 17;
    r_rtmid_img._dx = 23;
    r_rtmid_img._dy = 23;
    r_rtmid_img._xref = 11.5;
    r_rtmid_img._yref = 11.5;
    r_rtmid_img._rad = 11;

    r_fdamp_img._image [0] = revsect_img;
    r_fdamp_img._lncol [0] = 0;
    r_fdamp_img._x0 = 267;
    r_fdamp_img._y0 = 17;
    r_fdamp_img._dx = 23;
    r_fdamp_img._dy = 23;
    r_fdamp_img._xref = 11.5;
    r_fdamp_img._yref = 11.5;
    r_fdamp_img._rad = 11;

    r_parfr_img._image [0] = eq1sect_img;
    r_parfr_img._lncol [0] = 0;
    r_parfr_img._x0 = 19;
    r_parfr_img._y0 = 32;
    r_parfr_img._dx = 23;
    r_parfr_img._dy = 23;
    r_parfr_img._xref = 11.5;
    r_parfr_img._yref = 11.5;
    r_parfr_img._rad = 11;

    r_pargn_img._image [0] = eq1sect_img;
    r_pargn_img._lncol [0] = 1;
    r_pargn_img._x0 = 68;
    r_pargn_img._y0 = 17;
    r_pargn_img._dx = 23;
    r_pargn_img._dy = 23;
    r_pargn_img._xref = 11.5;
    r_pargn_img._yref = 11.5;
    r_pargn_img._rad = 11;

    r_opmix_img._image [0] = mixsect_img;
    r_opmix_img._lncol [0] = 0;
    r_opmix_img._x0 = 23;
    r_opmix_img._y0 = 32;
    r_opmix_img._dx = 23;
    r_opmix_img._dy = 23;
    r_opmix_img._xref = 11.5;
    r_opmix_img._yref = 11.5;
    r_opmix_img._rad = 11;

    r_rgxyz_img._image [0] = ambsect_img;
    r_rgxyz_img._lncol [0] = 0;
    r_rgxyz_img._x0 = 23;
    r_rgxyz_img._y0 = 32;
    r_rgxyz_img._dx = 23;
    r_rgxyz_img._dy = 23;
    r_rgxyz_img._xref = 11.5;
    r_rgxyz_img._yref = 11.5;
    r_rgxyz_img._rad = 11;
    
    return true;
}


void styles_fini ()
{
    revsect_img = 0;
    mixsect_img = 0;
    ambsect_img = 0;
    eq1sect_img = 0;
    eq2sect_img = 0;
    redzita_img = 0;
}
