#ifdef WIN32

#include <Windows.h>
#include <mmsystem.h>
#include "framework.h"
#include "internal.h"

void CALLBACK HandleMidiMessage(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

static HMIDIIN s_midiHandle = 0;

bool initMidi()
{
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
			return true;
		}
	}
}

void shutMidi()
{
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
			if (isDown != globals.midiDown[key])
			{
				globals.midiDown[key] = isDown;
				globals.midiChange[key] = true;
			}
			globals.midiValue[key] = volume * 2.0;
		}
		break;
	}
}

#else

bool initMidi()
{
}

void shutMidi()
{
}

#endif
