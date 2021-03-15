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


#ifndef __JCLIENT_H
#define __JCLIENT_H


#include <clthreads.h>
#include <jack/jack.h>
#include "global.h"
#include "reverb.h"


class Jclient : public A_thread
{
public:

    Jclient (const char *jname, const char *jserv, bool ambis);
    ~Jclient (void);

    const char *jname  (void) const { return _jname; }
    Reverb     *reverb (void) const { return (Reverb *) &_reverb; }

private:

    void  init_jack (const char *jname, const char *jserv);
    void  close_jack (void);
    void  jack_shutdown (void);
    int   jack_process (int nframes);

    virtual void thr_main (void) {}

    jack_client_t  *_jack_client;
    jack_port_t    *_inpports [2];
    jack_port_t    *_outports [4];
    bool            _active;
    const char     *_jname;
    unsigned int    _fsamp;
    bool            _ambis;
    int             _fragm;
    int             _nsamp;
    Reverb          _reverb;

    static void jack_static_shutdown (void *arg);
    static int  jack_static_process (jack_nframes_t nframes, void *arg);
};


#endif
