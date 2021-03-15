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

#include "styles.h"
#include "jclient.h"
#include "mainwin.h"

#include "audiooutput/AudioOutput_Native.h"
#include "framework.h"

#define SAMPLE_RATE 44100

int main (int ac, char *av [])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
    
    if (!framework.init(Mainwin::XSIZE, Mainwin::YSIZE))
		return -1;

	if (!styles_init())
		return -1;
    
    bool ambisonic = false;
    
    AudioOutput_Native audioOutput;
    audioOutput.Initialize(2, SAMPLE_RATE, 256);
    
    AudioStream_Reverb audioStream(SAMPLE_RATE, ambisonic);
    audioOutput.Play(&audioStream);
    
    Mainwin * mainwin = new Mainwin(&audioStream, ambisonic);

    for (;;)
    {
		framework.process();
		
		if (framework.quitRequested)
			break;
		
        mainwin->tick();
        
        framework.beginDraw(0, 0, 0, 0);
        {
			mainwin->draw();
		}
		framework.endDraw();
    }
    
    delete mainwin;
    mainwin = nullptr;
    
    audioOutput.Stop();
    
    styles_fini();
    
    framework.shutdown();

    return 0;
}



