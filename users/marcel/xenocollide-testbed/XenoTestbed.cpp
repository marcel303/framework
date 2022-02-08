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

#include "stdafx.h"
#include "XenoTestbed.h"
#include "XenoTestbedWindow.h"
#include "XenoTestbed.h"

#include "TestModule.h"

#include <mmsystem.h>

XenoTestbedApp::XenoTestbedApp()
: m_mainWindow(NULL)
{
}

XenoTestbedApp theApp;

BOOL XenoTestbedApp::PumpWaitingMessages()
{
	MSG msg;

	//
	// Read all of the messages in this next loop, 
	// removing each message as we read it.
	//
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		//
		// If it's a quit message, we're out of here.
		//
		if (msg.message == WM_QUIT)
		{
			return FALSE;
		}

		//
		// Otherwise, dispatch the message.
		//
		TranslateMessage(&msg);
		DispatchMessage(&msg); 
	}

	return TRUE;
}

BOOL XenoTestbedApp::RunMainLoop()
{
	DWORD currentTime = timeGetTime();

	while (PumpWaitingMessages())
	{
		DWORD newTime = timeGetTime();
		float32 dt = (float32)(newTime - currentTime) / 1000.0f;
		if (dt > 0)
		{
			m_mainWindow->Simulate(dt);
			currentTime = newTime;
		}
	}

	return TRUE;
}

BOOL XenoTestbedApp::InitInstance(HINSTANCE hInstance)
{
	void* mem = _aligned_malloc(sizeof(XenoTestbedWindow), 16);

	m_mainWindow = new(mem) XenoTestbedWindow(hInstance);

	if ( !m_mainWindow->Init() )
	{
		return FALSE;
	}

	return true;
}

int XenoTestbedApp::ExitInstance()
{
	// Clean up the main window
	m_mainWindow->~XenoTestbedWindow();		// !!! TODO: MAJOR HACK TO GET AROUND ALIGNMENT ISSUES !!!
	_aligned_free(m_mainWindow);
	m_mainWindow = NULL;

	// Clean up any globals
	RegisterTestModule::CleanUp();

	return TRUE;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// Perform application initialization:
	if (!theApp.InitInstance(hInstance))
	{
		return FALSE;
	}

	// Run the application
	BOOL result = theApp.RunMainLoop();

	// Perform application clean up
	theApp.ExitInstance();

	return result;
}
