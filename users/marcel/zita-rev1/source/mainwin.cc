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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "styles.h"
#include "global.h"
#include "mainwin.h"
#include "nsm.h"

extern NSM_Client *nsm;

Mainwin::Mainwin (X_rootwin *parent, X_resman *xres, int xp, int yp, Jclient *jclient, bool force_ambis) :
    A_thread ("Main"),
    X_window (parent, xp, yp, XSIZE, YSIZE, XftColors [C_MAIN_BG]->pixel),
    _stop (false),
    _xres (xres),
    _jclient (jclient),
    _dirty (false),
    _managed (false)
{
    X_hints     H;
    char        s [256];
    int         i, x;

    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);

    sprintf (s, "%s", jclient->jname ());
    x_set_title (s);
    H.position (xp, yp);
    H.minsize (XSIZE, YSIZE);
    H.maxsize (XSIZE, YSIZE);
    H.rname (xres->rname ());
    H.rclas (xres->rclas ());
    x_apply (&H); 

    _ambis = xres->getb (".ambisonic", force_ambis);
    RotaryCtl::init (disp ());
    x = 0;
    _rotary [R_DELAY] = new Rlinctl (this, this, &r_delay_img, x, 0, 160, 5,  0.02,  0.100,  0.04, R_DELAY);
    _rotary [R_XOVER] = new Rlogctl (this, this, &r_xover_img, x, 0, 200, 5,  50.0, 1000.0, 200.0, R_XOVER);
    _rotary [R_RTLOW] = new Rlogctl (this, this, &r_rtlow_img, x, 0, 200, 5,   1.0,    8.0,   3.0, R_RTLOW);
    _rotary [R_RTMID] = new Rlogctl (this, this, &r_rtmid_img, x, 0, 200, 5,   1.0,    8.0,   2.0, R_RTMID);
    _rotary [R_FDAMP] = new Rlogctl (this, this, &r_fdamp_img, x, 0, 200, 5, 1.5e3, 24.0e3, 6.0e3, R_FDAMP);
    x += 315;
    _rotary [R_EQ1FR] = new Rlogctl (this, this, &r_parfr_img, x, 0, 180, 5,  40.0,  2.5e3, 160.0, R_EQ1FR);
    _rotary [R_EQ1GN] = new Rlinctl (this, this, &r_pargn_img, x, 0, 150, 5, -15.0,   15.0,   0.0, R_EQ1GN);
    x += 110;
    _rotary [R_EQ2FR] = new Rlogctl (this, this, &r_parfr_img, x, 0, 180, 5, 160.0,   10e3, 2.5e3, R_EQ2FR);
    _rotary [R_EQ2GN] = new Rlinctl (this, this, &r_pargn_img, x, 0, 150, 5, -15.0,   15.0,   0.0, R_EQ2GN);
    x += 110;
    _rotary [R_OPMIX] = new Rlinctl (this, this, &r_opmix_img, x, 0, 180, 5,   0.0 ,   1.0,   0.5, R_OPMIX);
    _rotary [R_RGXYZ] = new Rlinctl (this, this, &r_rgxyz_img, x, 0, 180, 5,  -9.0 ,   9.0,   0.0, R_RGXYZ);
    for (i = 0; i < R_OPMIX; i++) _rotary [i]->x_map ();
    if (_ambis) _rotary [R_RGXYZ]->x_map ();
    else        _rotary [R_OPMIX]->x_map ();

    _jclient->reverb ()->set_delay (_rotary [R_DELAY]->value ());
    _jclient->reverb ()->set_xover (_rotary [R_XOVER]->value ());
    _jclient->reverb ()->set_rtlow (_rotary [R_RTLOW]->value ());
    _jclient->reverb ()->set_rtmid (_rotary [R_RTMID]->value ());
    _jclient->reverb ()->set_fdamp (_rotary [R_FDAMP]->value ());
    _jclient->reverb ()->set_opmix (_rotary [R_OPMIX]->value ());
    _jclient->reverb ()->set_rgxyz (_rotary [R_RGXYZ]->value ());
    _jclient->reverb ()->set_eq1 (_rotary [R_EQ1FR]->value (), _rotary [R_EQ1GN]->value ());
    _jclient->reverb ()->set_eq2 (_rotary [R_EQ2FR]->value (), _rotary [R_EQ2GN]->value ());

    x_add_events (ExposureMask); 
    x_map (); 
    set_time (0);
    inc_time (500000);
}

 
Mainwin::~Mainwin (void)
{
    RotaryCtl::fini ();
}

 
int Mainwin::process (void)
{
    int e;

    if (_stop) handle_stop ();

    e = get_event_timed ();
    switch (e)
    {
    case EV_TIME:
        handle_time ();
        break;
    }
    return e;
}


void Mainwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
        expose ((XExposeEvent *) E);
        break;

    case ClientMessage:
        clmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Mainwin::expose (XExposeEvent *E)
{
    if (E->count) return;
    redraw ();
}


void Mainwin::clmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) _stop = true;
}


void Mainwin::handle_time (void)
{
    inc_time (500000);
//    XFlush (dpy ());
}


void Mainwin::handle_stop (void)
{
    put_event (EV_EXIT, 1);
}


void Mainwin::handle_callb (int type, X_window *W, XEvent *E)
{
    RotaryCtl  *R;
    int         k;

    switch (type)
    {
    case RotaryCtl::PRESS:
        R = (RotaryCtl *) W;
        k = R->cbind ();
        switch (k)
        {
        default:
            ;
        }
        break;

    case RotaryCtl::DELTA:
        R = (RotaryCtl *) W;
        k = R->cbind ();
        switch (k)
        {
        case R_DELAY:
            _jclient->reverb ()->set_delay (_rotary [R_DELAY]->value ());
            break;
        case R_XOVER:
            _jclient->reverb ()->set_xover (_rotary [R_XOVER]->value ());
            break;
        case R_RTLOW:
            _jclient->reverb ()->set_rtlow (_rotary [R_RTLOW]->value ());
            break;
        case R_RTMID:
            _jclient->reverb ()->set_rtmid (_rotary [R_RTMID]->value ());
            break;
        case R_FDAMP:
            _jclient->reverb ()->set_fdamp (_rotary [R_FDAMP]->value ());
            break;
        case R_OPMIX:
            _jclient->reverb ()->set_opmix (_rotary [R_OPMIX]->value ());
            break;
        case R_RGXYZ:
            _jclient->reverb ()->set_rgxyz (_rotary [R_RGXYZ]->value ());
            break;
        case R_EQ1FR:
        case R_EQ1GN:
            _jclient->reverb ()->set_eq1 (_rotary [R_EQ1FR]->value (), _rotary [R_EQ1GN]->value ());
            break;
        case R_EQ2FR:
        case R_EQ2GN:
            _jclient->reverb ()->set_eq2 (_rotary [R_EQ2FR]->value (), _rotary [R_EQ2GN]->value ());
            break;
        }
        break;
    }

    if (!_dirty)
    {
        if (nsm) nsm->is_dirty ();
        _dirty = true;
    }
}


void Mainwin::redraw (void)
{
    int x;

    x = 0;
    XPutImage (dpy (), win (), dgc (), revsect_img, 0, 0,   x, 0, 310, 75);
    x += 315;
    XPutImage (dpy (), win (), dgc (), eq1sect_img, 0, 0,   x, 0, 110, 75);
    x += 110;
    XPutImage (dpy (), win (), dgc (), eq2sect_img, 0, 0,   x, 0, 110, 75);
    x += 110;
    if (_ambis) XPutImage (dpy (), win (), dgc (), ambsect_img, 0, 0, x, 0, 70, 75);
    else        XPutImage (dpy (), win (), dgc (), mixsect_img, 0, 0, x, 0, 70, 75);
    x += 70;
    XPutImage (dpy (), win (), dgc (), redzita_img, 0, 0,   x, 0, 35, 75);
    if (_managed)
    {
        x += 10;
        XPutImage (dpy (), win (), dgc (), sm_img,      0, 0,   x, 60, 19, 10);
    }
}


void Mainwin::load_state ()
{
    FILE * File;
    File = fopen (_statefile, "r");

    if (File != NULL)
    {
        char parameter [20];
        float delay = 0.0f;
        float xover = 0.0f;
        float rtlow = 0.0f;
        float rtmid = 0.0f;
        float fdamp = 0.0f;
        float opmix = 0.0f;
        float rgxyz = 0.0f;
        float eq1fr = 0.0f;
        float eq1gn = 0.0f;
        float eq2fr = 0.0f;
        float eq2gn = 0.0f;
        int xp, yp;

        fscanf (File, "%s %f %s %f %s %f %s %f %s %f %s %f %s %f %s %f %s %f %s %f %s %f %s %d %s %d",
                parameter, &delay,
                parameter, &xover,
                parameter, &rtlow,
                parameter, &rtmid,
                parameter, &fdamp,
                parameter, &opmix,
                parameter, &rgxyz,
                parameter, &eq1fr,
                parameter, &eq1gn,
                parameter, &eq2fr,
                parameter, &eq2gn,
                parameter, &xp,
                parameter, &yp);
        fclose (File);

        _rotary [R_DELAY]->set_value (delay);
        _jclient->reverb ()->set_delay (_rotary [R_DELAY]->value ());
        _rotary [R_XOVER]->set_value (xover);
        _jclient->reverb ()->set_xover (_rotary [R_XOVER]->value ());
        _rotary [R_RTLOW]->set_value (rtlow);
        _jclient->reverb ()->set_rtlow (_rotary [R_RTLOW]->value ());
        _rotary [R_RTMID]->set_value (rtmid);
        _jclient->reverb ()->set_rtmid (_rotary [R_RTMID]->value ());
        _rotary [R_FDAMP]->set_value (fdamp);
        _jclient->reverb ()->set_fdamp (_rotary [R_FDAMP]->value ());
        _rotary [R_OPMIX]->set_value (opmix);
        _jclient->reverb ()->set_opmix (_rotary [R_OPMIX]->value ());
        _rotary [R_RGXYZ]->set_value (rgxyz);
        _jclient->reverb ()->set_rgxyz (_rotary [R_RGXYZ]->value ());
        _rotary [R_EQ1FR]->set_value (eq1fr);
        _rotary [R_EQ1GN]->set_value (eq1gn);
        _jclient->reverb ()->set_eq1 (_rotary [R_EQ1FR]->value (), _rotary [R_EQ1GN]->value ());
        _rotary [R_EQ2FR]->set_value (eq2fr);
        _rotary [R_EQ2GN]->set_value (eq2gn);
        _jclient->reverb ()->set_eq2 (_rotary [R_EQ2FR]->value (), _rotary [R_EQ2GN]->value ());
        x_move (xp, yp);
        redraw ();
    }
}


void Mainwin::save_state (void)
{
    FILE * File;
    File = fopen (_statefile, "w");

    if (File != NULL)
    {
        float delay = _rotary [R_DELAY]->value ();
        float xover = _rotary [R_XOVER]->value ();
        float rtlow = _rotary [R_RTLOW]->value ();
        float rtmid = _rotary [R_RTMID]->value ();
        float fdamp = _rotary [R_FDAMP]->value ();
        float opmix = _rotary [R_OPMIX]->value ();
        float rgxyz = _rotary [R_RGXYZ]->value ();
        float eq1fr = _rotary [R_EQ1FR]->value ();
        float eq1gn = _rotary [R_EQ1GN]->value ();
        float eq2fr = _rotary [R_EQ2FR]->value ();
        float eq2gn = _rotary [R_EQ2GN]->value ();

        fprintf (File, "/reverb/delay\t%f\n/reverb/xover\t%f\n", delay, xover);
        fprintf (File, "/reverb/rtlow\t%f\n/reverb/rtmid\t%f\n", rtlow, rtmid);
        fprintf (File, "/reverb/fdamp\t%f\n/reverb/opmix\t%f\n", fdamp, opmix);
        fprintf (File, "/reverb/rgxyz\t%f\n/reverb/eq1fr\t%f\n", rgxyz, eq1fr);
        fprintf (File, "/reverb/eq1gn\t%f\n/reverb/eq2fr\t%f\n", eq1gn, eq2fr);
        fprintf (File, "/reverb/eq2gn\t%f\n", eq2gn);

        Window w_return;
        int x_s, y_s, x, y;
        unsigned int w, h;
        unsigned int b_w;
        unsigned int d;

        XGetGeometry (dpy (), win (), &w_return, &x, &y, &w, &h, &b_w, &d);
        XTranslateCoordinates (dpy (), win (), pwin ()->win (),
                               -x, -y, &x_s, &y_s, &w_return);

        fprintf(File, "/window/x\t%d\n/window/y\t%d\n", x_s, y_s);
        fclose (File);
        _dirty = false;
        if (nsm) nsm->is_clean();
    }
}


void Mainwin::set_managed (bool m)
{
    _managed = m;
    redraw ();
}


