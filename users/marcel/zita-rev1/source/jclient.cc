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


#include "jclient.h"


Jclient::Jclient (const char *jname, const char *jserv, bool ambis) :
    A_thread ("Jclient"),
    _jack_client (0),
    _active (false),
    _jname (0),
    _ambis (ambis)
{
    init_jack (jname, jserv);   
}


Jclient::~Jclient (void)
{
    if (_jack_client) close_jack ();
}


void Jclient::init_jack (const char *jname, const char *jserv)
{
    jack_status_t  stat;
    int            opts;

    opts = JackNoStartServer;
    if (jserv) opts |= JackServerName;
    if ((_jack_client = jack_client_open (jname, (jack_options_t) opts, &stat, jserv)) == 0)
    {
        fprintf (stderr, "Can't connect to JACK\n");
        exit (1);
    }
    jack_set_process_callback (_jack_client, jack_static_process, (void *) this);
    jack_on_shutdown (_jack_client, jack_static_shutdown, (void *) this);
    if (jack_activate (_jack_client))
    {
        fprintf(stderr, "Can't activate JACK.\n");
        exit (1);
    }
    _jname = jack_get_client_name (_jack_client);
    _fsamp = jack_get_sample_rate (_jack_client);

    _fragm = 1024;
    _nsamp = 0;

    if (_ambis)
    {
        _inpports [0] = jack_port_register (_jack_client, "in.L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        _inpports [1] = jack_port_register (_jack_client, "in.R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        _outports [0] = jack_port_register (_jack_client, "out.W", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        _outports [1] = jack_port_register (_jack_client, "out.X", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        _outports [2] = jack_port_register (_jack_client, "out.Y", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        _outports [3] = jack_port_register (_jack_client, "out.Z", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }
    else
    {
        _inpports [0] = jack_port_register (_jack_client, "in.L",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        _inpports [1] = jack_port_register (_jack_client, "in.R",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        _outports [0] = jack_port_register (_jack_client, "out.L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        _outports [1] = jack_port_register (_jack_client, "out.R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }

    _reverb.init (_fsamp, _ambis);
    _active = true;
}


void Jclient::close_jack ()
{
    jack_deactivate (_jack_client);
    jack_client_close (_jack_client);
}


void Jclient::jack_static_shutdown (void *arg)
{
    ((Jclient *) arg)->jack_shutdown ();
}


int Jclient::jack_static_process (jack_nframes_t nframes, void *arg)
{
    return ((Jclient *) arg)->jack_process (nframes);
}


void Jclient::jack_shutdown (void)
{
    send_event (EV_EXIT, 1);
}


int Jclient::jack_process (int frames)
{
    int   i, k, n_inp, n_out;
    float *inp [2];
    float *out [4];

    if (!_active) return 0;

    n_inp = 2;
    n_out = _ambis ? 4 : 2;
    for (i = 0; i < n_inp; i++) inp [i] = (float *) jack_port_get_buffer (_inpports [i], frames);
    for (i = 0; i < n_out; i++) out [i] = (float *) jack_port_get_buffer (_outports [i], frames);

    while (frames)
    {
        if (!_nsamp)
        {
            _reverb.prepare (_fragm);
            _nsamp = _fragm;
        }
        k = (_nsamp < frames) ? _nsamp : frames;
        _reverb.process (k, inp, out);
        for (i = 0; i < n_inp; i++) inp [i] += k;
        for (i = 0; i < n_out; i++) out [i] += k;
        frames -= k;
        _nsamp -= k;
    }


    return 0;
}

