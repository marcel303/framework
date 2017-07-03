/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef WIN32

#include <Windows.h>
#include <mmsystem.h>
#include "framework.h"
#include "internal.h"

void CALLBACK HandleMidiMessage(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

static HMIDIIN s_midiHandle = 0;
static HANDLE s_midiMutex = 0;

void lockMidi()
{
	if (s_midiMutex != 0)
	{
		if (WaitForSingleObject(s_midiMutex, INFINITE) != WAIT_OBJECT_0)
			logError("lockMidi error. error=%x", GetLastError());
	}
}

void unlockMidi()
{
	if (s_midiMutex != 0)
	{
		if (!ReleaseMutex(s_midiMutex))
			logError("unlockMidi error. error=%x", GetLastError());
	}
}

bool initMidi(int deviceIndex)
{
	MMRESULT mmResult = S_OK;

	memset(globals.midiChange, 0, sizeof(globals.midiChange));
	memset(globals.midiChangeAsync, 0, sizeof(globals.midiChangeAsync));
	memset(globals.midiDown, 0, sizeof(globals.midiDown));
	memset(globals.midiDownAsync, 0, sizeof(globals.midiDownAsync));
	memset(globals.midiValue, 0, sizeof(globals.midiValue));

	mmResult = midiInOpen(&s_midiHandle, deviceIndex, (DWORD)HandleMidiMessage, 0, CALLBACK_FUNCTION);

	if (mmResult != S_OK)
	{
		logDebug("failed to open MIDI input. mmresult=%x, GetLastError=%x", mmResult, GetLastError());
		return false;
	}
	else
	{
		mmResult = midiInStart(s_midiHandle);

		if (mmResult != S_OK)
		{
			logError("failed to start MIDI input. mmresult=%x, GetLastError=%x", mmResult, GetLastError());
			return false;
		}
		else
		{
			midi.isConnected = true;

			s_midiMutex = CreateMutex(NULL, false, NULL);

			if (s_midiMutex == NULL)
			{
				logError("failed to create MIDI mutex. GetLastError=%x", GetLastError());
				return false;
			}
			else
			{
				return true;
			}
		}
	}
}

void shutMidi()
{
	if (s_midiMutex != 0)
	{
		if (!CloseHandle(s_midiMutex))
			logError("failed to destroy MIDI mutex. GetLastError=%x", GetLastError());
		s_midiMutex = 0;
	}

	if (midi.isConnected)
	{
		const MMRESULT mmResult = midiInStop(s_midiHandle);
		if (mmResult != S_OK)
			logError("failed to stop MIDI input. mmresult=%x, GetLastError=%x", mmResult, GetLastError());
		midi.isConnected = false;
	}

	if (s_midiHandle != 0)
	{
		const MMRESULT mmResult = midiInClose(s_midiHandle);
		if (mmResult != S_OK)
			logError("failed to close MIDI input. mmresult=%x, GetLastError=%x", mmResult, GetLastError());
		s_midiHandle = 0;
	}
}

void CALLBACK HandleMidiMessage(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	lockMidi();

	switch (uMsg)
	{
	case MIM_OPEN:
	case MIM_CLOSE:
	case MIM_LONGDATA:
		break;

	case MIM_MOREDATA:
	case MIM_ERROR:
	case MIM_LONGERROR:
		logWarning("received midi message msg=0x%08x, inst=%x, param1=%x, param2=%x", uMsg, dwInstance, dwParam1, dwParam2);
		break;

	case MIM_DATA:
		{
			const int volume = (dwParam1 & 0x00FF0000) >> 16;
			const int key = (dwParam1 & 0x0000FF00) >> 8;
			const bool isDown = (volume != 0);
			if (isDown != globals.midiDownAsync[key])
			{
				globals.midiDownAsync[key] = isDown;
				globals.midiChangeAsync[key] = true;
			}
			globals.midiValue[key] = volume / 127.f;
			globals.midiIsSetAsync[key] = true;
			//logDebug("MIDI data: key=%03d, isDown=%d, value=%03d", key, isDown, volume);
		}
		break;
	}

	unlockMidi();
}

#else

void lockMidi()
{
}

void unlockMidi()
{
}

bool initMidi(int deviceIndex)
{
    return true;
}

void shutMidi()
{
}

#endif
