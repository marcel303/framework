#include "config.h"
#include "rtmidi/RtMidi.h"

#define MAX_MIDI_KEYS 128

class Midi
{
	RtMidiIn * in = nullptr;

	bool midiIsSet[MAX_MIDI_KEYS];
	bool midiDown[MAX_MIDI_KEYS];
	bool midiChange[MAX_MIDI_KEYS];
	float midiValue[MAX_MIDI_KEYS];

public:
	bool isConnected = false;

	bool isDown(int key) const;
	bool wentDown(int key) const;
	bool wentUp(int key) const;
	float getValue(int key, float _default) const;

	Midi();
	~Midi();
	
	bool init(const int portIndex);

	void process();
};

class MappedMidi
{
	Midi & midi;
	Config & config;

public:
	MappedMidi(Midi & in_midi, Config & in_config);

	bool midiIsDown(int id) const;
	bool midiWentDown(int id) const;
	bool midiWentUp(int id) const;
	float midiGetValue(int id, float _default) const;
};
