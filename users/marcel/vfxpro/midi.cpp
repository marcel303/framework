#include "Log.h"
#include "midi.h"
#include <string.h>

static const uint8_t kMidi_NoteOff = 0x80;
static const uint8_t kMidi_NoteOn = 0x90;

Midi::Midi()
{
	memset(midiIsSet, 0, sizeof(midiIsSet));
	memset(midiChange, 0, sizeof(midiChange));
	memset(midiDown, 0, sizeof(midiDown));
	memset(midiValue, 0, sizeof(midiValue));
}

Midi::~Midi()
{
	delete in;
	in = nullptr;
}

bool Midi::init(const int portIndex)
{
	in = new RtMidiIn(RtMidi::UNSPECIFIED, "Midi Controller", 1024);
	
	for (int i = 0; i < in->getPortCount(); ++i)
	{
		auto name = in->getPortName();
		
		LOG_DBG("available MIDI port: %d: %s", i, name.c_str());
	}
	
	if (portIndex < in->getPortCount())
	{
		in->openPort(portIndex);
	}
	
	return in->isPortOpen();
}

void Midi::process()
{
	memset(midiChange, 0, sizeof(midiChange));
	
	if (in->isPortOpen())
	{
		for (;;)
		{
			std::vector<uint8_t> bytes;
			in->getMessage(&bytes);
			
			if (bytes.empty())
				break;

			if (bytes.size() == 3)
			{
				const uint8_t message = bytes[0] & 0xF0;
				const uint8_t keyIndex = bytes[1];
				const uint8_t velocity = bytes[2];
				
				if (message == kMidi_NoteOn)
				{
					if (keyIndex >= 0 && keyIndex < MAX_MIDI_KEYS)
					{
						midiIsSet[keyIndex] = true;
						midiDown[keyIndex] = velocity != 0;
						midiChange[keyIndex] = true;
						midiValue[keyIndex] = velocity;
					}
				}
				else if (message == kMidi_NoteOff)
				{
					if (keyIndex >= 0 && keyIndex < MAX_MIDI_KEYS)
					{
						midiIsSet[keyIndex] = true;
						midiDown[keyIndex] = false;
						midiChange[keyIndex] = true;
						midiValue[keyIndex] = velocity;
					}
				}
			}
		}
	}
}

bool Midi::isDown(int key) const
{
	if (key >= 0 && key < MAX_MIDI_KEYS)
		return midiDown[key];
	else
		return false;
}

bool Midi::wentDown(int key) const
{
	if (key >= 0 && key < MAX_MIDI_KEYS)
		return midiDown[key] && midiChange[key];
	else
		return false;
}

bool Midi::wentUp(int key) const
{
	if (key >= 0 && key < MAX_MIDI_KEYS)
		return !midiDown[key] && midiChange[key];
	else
		return false;
}

float Midi::getValue(int key, float _default) const
{
	if (midiIsSet[key] && isDown(key))
		return midiValue[key];
	else
		return _default;
}

//

MappedMidi::MappedMidi(Midi & in_midi, Config & in_config)
	: midi(in_midi)
	, config(in_config)
{
}

bool MappedMidi::midiIsDown(int id) const
{
	if (!config.midiIsMapped(id))
		return false;
	return midi.isDown(config.midi.mapping[id]);
}

bool MappedMidi::midiWentDown(int id) const
{
	if (!config.midiIsMapped(id))
		return false;
	return midi.wentDown(config.midi.mapping[id]);
}

bool MappedMidi::midiWentUp(int id) const
{
	if (!config.midiIsMapped(id))
		return false;
	return midi.wentUp(config.midi.mapping[id]);
}

float MappedMidi::midiGetValue(int id, float _default) const
{
	if (!config.midiIsMapped(id))
		return _default;
	return midi.getValue(config.midi.mapping[id], _default);
}
