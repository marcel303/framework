#pragma once

#include <math.h>

namespace MidiDecoder
{
	enum MessageType
	{
		kMessageType_None = 0,
		kMessageType_NoteOn = 0x90,
		kMessageType_NoteOff = 0x80,
		kMessageType_PolyKeyPressure = 0xa0,
		kMessageType_ControllerChange = 0xb0,
		kMessageType_PitchBend = 0xe0
	};

	struct Message
	{
		struct NoteOn
		{
			int note;
			int velocity;
		};

		struct NoteOff
		{
			int note;
		};

		struct ControllerChange
		{
			int note;
			int value;
		};

		MessageType type;

		union
		{
			NoteOn noteOn;
			NoteOff noteOff;
			ControllerChange controllerChange;
		};

		bool decode(const uint8_t * message, const int messageLength)
		{
			memset(this, 0, sizeof(*this));

			if (messageLength >= 1)
			{
				const uint8_t b = message[0];
				
				int channel;
				int event;
				
				if ((b & 0xf0) != 0xf0)
				{
					channel = b & 0x0f;
					event = b & 0xf0;
				}
				else
				{
					channel = 0;
					event = b;
				}
				
				if (event == kMessageType_ControllerChange && messageLength >= 3)
				{
					const int key = message[1];
					const int value = message[2];
					
					type = (MessageType)event;

					controllerChange.note = key;
					controllerChange.value = value;
				}
				else if (event == kMessageType_NoteOn && messageLength >= 3)
				{
					const int key = message[1];
					const int value = message[2];
					
					type = (MessageType)event;

					noteOn.note = key;
					noteOn.velocity = value;
				}
				else if (event == kMessageType_NoteOff && messageLength >= 2)
				{
					// todo : check if note off is implemented correctly. so far my MIDI devices don't seem to trigger OFF events
					Assert(false);

					const int key = message[1];
					
					type = (MessageType)event;

					noteOff.note = key;
				}
				else if (event == kMessageType_PitchBend)
				{
					// todo : implement
					Assert(false);
				}
			}

			return type != kMessageType_None;
		}
	};

	static float midiNoteToFrequency(const int note)
	{
		return powf(2.f, (note - 69) / 12.f) * 440.f;
	}
}
