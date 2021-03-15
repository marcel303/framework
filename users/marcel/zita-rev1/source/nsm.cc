
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/


#include "nsm.h"
#include "mainwin.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern Mainwin    *mainwin;

NSM_Client::NSM_Client()
{
}

int
NSM_Client::command_save(char **out_msg)
{
    (void) out_msg;
    int r = ERR_OK;

    mainwin->save_state ();

    return r;
}

/*int
NSM_Client::command_open(const char *name,
                         const char *display_name,
                         const char *client_id,
                         char **out_msg)
{
    int r = ERR_OK;
    return r;
}

void
NSM_Client::command_active(bool active)
{
    ;
}*/
