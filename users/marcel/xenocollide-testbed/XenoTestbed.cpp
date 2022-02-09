/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

// XenoTestbed.cpp : Defines the class behaviors for the application.
//

#include "XenoTestbed.h"
#include "XenoTestbedWindow.h"
#include "XenoTestbed.h"

#include "TestModule.h"

#include "framework.h"

XenoTestbedApp::XenoTestbedApp()
: m_mainWindow(NULL)
{
}

XenoTestbedApp theApp;

bool XenoTestbedApp::RunMainLoop()
{
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		m_mainWindow->Check();
		
		float32 dt = framework.timeStep;
		
		if (dt > 0)
		{
			m_mainWindow->Simulate(dt);
			
			m_mainWindow->OnPaint();
		}
	}

	return true;
}

bool XenoTestbedApp::InitInstance()
{
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	framework.init(800, 600);
	
	void* mem = _mm_malloc(sizeof(XenoTestbedWindow), 16);

	m_mainWindow = new(mem) XenoTestbedWindow();

	if ( !m_mainWindow->Init() )
	{
		return false;
	}

	return true;
}

int XenoTestbedApp::ExitInstance()
{
	// Clean up the main window
	m_mainWindow->~XenoTestbedWindow();		// !!! TODO: MAJOR HACK TO GET AROUND ALIGNMENT ISSUES !!!
	_mm_free(m_mainWindow);
	m_mainWindow = NULL;

	// Clean up any globals
	RegisterTestModule::CleanUp();

	framework.shutdown();
	
	return true;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	// Perform application initialization:
	if (!theApp.InitInstance())
	{
		return -1;
	}

	// Run the application
	bool result = theApp.RunMainLoop();

	// Perform application clean up
	theApp.ExitInstance();

	return result ? 0 : -1;
}
