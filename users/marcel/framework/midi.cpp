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

bool initMidi()
{
	memset(globals.midiChange, 0, sizeof(globals.midiChange));
	memset(globals.midiChangeAsync, 0, sizeof(globals.midiChangeAsync));
	memset(globals.midiDown, 0, sizeof(globals.midiDown));
	memset(globals.midiDownAsync, 0, sizeof(globals.midiDownAsync));
	memset(globals.midiValue, 0, sizeof(globals.midiValue));

	if (midiInOpen(&s_midiHandle, 0, (DWORD)HandleMidiMessage, 0, CALLBACK_FUNCTION) != S_OK)
	{
		logDebug("failed to open MIDI input");
		return false;
	}
	else
	{
		if (midiInStart(s_midiHandle) != S_OK)
		{
			logError("failed to start MIDI input");
			return false;
		}
		else
		{
			midi.isConnected = true;

			s_midiMutex = CreateMutex(NULL, false, NULL);

			if (s_midiMutex == NULL)
			{
				logError("failed to create MIDI mutex");
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
		CloseHandle(s_midiMutex);
		s_midiMutex = 0;
	}

	if (midi.isConnected)
	{
		if (midiInStop(s_midiHandle) != S_OK)
			logError("failed to stop MIDI input");
		midi.isConnected = false;
	}

	if (s_midiHandle != 0)
	{
		if (midiInClose(s_midiHandle) != S_OK)
			logError("failed to close MIDI input");
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
		logWarning("received midi message 0x%08x", uMsg);
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
			logDebug("MIDI data: key=%03d, isDown=%d, value=%03d", key, isDown, volume);
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

bool initMidi()
{
}

void shutMidi()
{
}

#endif
