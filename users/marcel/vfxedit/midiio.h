#pragma once

struct MidiNote
{
	int value;
	float time;
};

bool midiWrite(const char * filename, const MidiNote * notes, int numNotes, int bpm);
