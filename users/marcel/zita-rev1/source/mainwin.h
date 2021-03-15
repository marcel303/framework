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


#ifndef __MAINWIN_H
#define        __MAINWIN_H


#include <clxclient.h>
#include "guiclass.h"
#include "jclient.h"
#include "global.h"


class Mainwin : public A_thread, public X_window, public X_callback
{
public:

    enum { XSIZE = 640, YSIZE = 75 };

    Mainwin (X_rootwin *parent, X_resman *xres, int xp, int yp, Jclient *jclient, bool force_ambis = false);
    ~Mainwin (void);
    Mainwin (const Mainwin&);
    Mainwin& operator=(const Mainwin&);

    void stop (void) { _stop = true; }
    int process (void);
    void load_state (void);
    void save_state (void);
    void set_managed (bool);
    void set_statefile (const char *s) { sprintf(_statefile, "%s", s); }

private:

    enum { R_DELAY, R_XOVER, R_RTLOW, R_RTMID, R_FDAMP,
           R_EQ1FR, R_EQ1GN, R_EQ2FR, R_EQ2GN,
           R_OPMIX, R_RGXYZ, NROTARY };
 
    virtual void thr_main (void) {}

    void handle_time (void);
    void handle_stop (void);
    void handle_event (XEvent *);
    void handle_callb (int type, X_window *W, XEvent *E);
    void expose (XExposeEvent *E);
    void clmesg (XClientMessageEvent *E);
    void redraw (void);

    Atom            _atom;
    bool            _stop;
    bool            _ambis;
    X_resman       *_xres;
    Jclient        *_jclient;
    RotaryCtl      *_rotary [NROTARY];
    char            _statefile [1024];
    bool            _dirty;
    bool            _managed;
};


#endif
