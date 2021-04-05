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

#include "framework.h"

extern GxTextureId revsect_img;
extern GxTextureId eq1sect_img;
extern GxTextureId eq2sect_img;
extern GxTextureId mixsect_img;
extern GxTextureId ambsect_img;
extern GxTextureId redzita_img;
extern GxTextureId sm_img;

extern RotaryImg r_delay_img;
extern RotaryImg r_xover_img;
extern RotaryImg r_rtlow_img;
extern RotaryImg r_rtmid_img;
extern RotaryImg r_fdamp_img;
extern RotaryImg r_parfr_img;
extern RotaryImg r_pargn_img;
extern RotaryImg r_opmix_img;
extern RotaryImg r_rgxyz_img;

bool styles_init();
void styles_fini();
