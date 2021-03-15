// ----------------------------------------------------------------------
//
//  Copyright (C) 2010-2011 Fons Adriaensen <fons@linuxaudio.org>
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
#include <clthreads.h>
#include <sys/mman.h>
#include <signal.h>
#include "global.h"
#include "styles.h"
#include "jclient.h"
#include "mainwin.h"
#include "nsm.h"


#define NOPTS 4
#define CP (char *)


XrmOptionDescRec options [NOPTS] =
{
    {CP"-h",   CP".help",      XrmoptionNoArg,   CP"true" },
    {CP"-g",   CP".geometry",  XrmoptionSepArg,  0        },
    {CP"-B",   CP".ambisonic", XrmoptionNoArg,   CP"true" },
    {CP"-s",   CP".server",    XrmoptionSepArg,  0        }
};



static Jclient  *jclient = 0;
Mainwin  *mainwin = 0;
NSM_Client *nsm = 0;


static void help (void)
{
    fprintf (stderr, "\n%s-%s\n\n", PROGNAME, VERSION);
    fprintf (stderr, "  (C) 2010-2011 Fons Adriaensen  <fons@linuxaudio.org>\n\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h              Display this text\n");
    fprintf (stderr, "  -B              Ambisonic mode\n");
    fprintf (stderr, "  -name <name>    Jack client name\n");
    fprintf (stderr, "  -s <server>     Jack server name\n");
    fprintf (stderr, "  -g <geometry>   Window position\n");
    exit (1);
}


static void sigint_handler (int)
{
    signal (SIGINT, SIG_IGN);
    mainwin->stop ();
}


int main (int ac, char *av [])
{
    X_resman       xresman;
    X_display     *display;
    X_handler     *handler;
    X_rootwin     *rootwin;
    int           ev, xp, yp, xs, ys;
    char          *nsm_url;
    char          program_name [32] = PROGNAME;
    char          state_file [1024] ="";
    bool          managed = false;
    bool          ambisonic = false;

    nsm_url = getenv("NSM_URL");

    if (nsm_url)
    {
        nsm = new NSM_Client;
        if (!nsm->init(nsm_url))
        {
            nsm->announce(av[0], ":dirty:", av[0]);
            do
            {
                nsm->check ();
                usleep(10);
                managed = nsm->is_active();
            } while (!nsm->is_active());
            do
            {
                nsm->check ();
                usleep(10);
            } while (!nsm->client_id());
            sprintf(program_name, "%s", nsm->client_id ());
            sprintf(state_file, "%s.conf", nsm->client_path ());
        }
        else
        {
            delete nsm;
            nsm = NULL;
        }
    }

    xresman.init (&ac, av, CP PROGNAME, options, NOPTS);
    if (xresman.getb (".help", 0)) help ();
            
    display = new X_display (xresman.get (".display", 0));
    if (display->dpy () == 0)
    {
        fprintf (stderr, "Can't open display.\n");
        delete display;
        return 1;
    }

    xp = yp = 100;
    xs = Mainwin::XSIZE + 4;
    ys = Mainwin::YSIZE + 30;
    xresman.geometry (".geometry", display->xsize (), display->ysize (), 1, xp, yp, xs, ys);

    styles_init (display, &xresman);

    if (strcmp(av[0], "zita-rev1-amb") == 0)
    {
        ambisonic = true;
    }

    if (managed)
    {
        jclient = new Jclient (program_name,
                               xresman.get (".server", 0),
                                xresman.getb (".ambisonic", ambisonic));
    }
    else
    {
        jclient = new Jclient (xresman.rname (),
                               xresman.get (".server", 0),
                                xresman.getb (".ambisonic", ambisonic));
    }

    rootwin = new X_rootwin (display);
    mainwin = new Mainwin (rootwin, &xresman, xp, yp, jclient, xresman.getb (".ambisonic", ambisonic));
    rootwin->handle_event ();
    handler = new X_handler (display, mainwin, EV_X11);
    handler->next_event ();
    XFlush (display->dpy ());
    ITC_ctrl::connect (jclient, EV_EXIT, mainwin, EV_EXIT);

    if (mlockall (MCL_CURRENT | MCL_FUTURE)) fprintf (stderr, "Warning: memory lock failed.\n");
    signal (SIGINT, sigint_handler); 

    mainwin->set_managed (managed);
    if (managed)
    {
        mainwin->set_statefile (state_file);
        mainwin->load_state ();
    }

    do
    {
        ev = mainwin->process ();
        if (ev == EV_X11)
        {
            rootwin->handle_event ();
            handler->next_event ();
        }
        if (ev == Esync::EV_TIME)
        {
            handler->next_event ();
        }
        if (nsm) nsm->check ();
    }
    while (ev != EV_EXIT);

    styles_fini (display);
    delete jclient;
    delete handler;
    delete rootwin;
    delete display;
    if (nsm) delete nsm;

    return 0;
}



