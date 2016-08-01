#include "FileStream.h"
#include "midifile/MidiFile.h"
#include "midiio.h"
#include "StreamWriter.h"

bool midiWrite(const char * filename, const MidiNote * notes, int numNotes)
{
	MidiFile file;
	file.absoluteTicks();
	file.addTrack(1);
	int tpq = 120;
	file.setTicksPerQuarterNote(tpq);

	for (int i = 0; i < numNotes; ++i)
	{
		int tick = int(double(notes[i].time) * tpq * 2);

		file.addNoteOn(0, tick, 0, notes[i].value, 100);
		file.addNoteOff(0, tick + 1, 0, notes[i].value, 100);
	}

	file.sortTracks();

	if (file.write(filename) == 0)
		return false;

	return true;
}
