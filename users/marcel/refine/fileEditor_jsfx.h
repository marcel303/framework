#pragma once

#include "fileEditor.h"
#include "gfx-framework.h"
#include "jsusfx_file.h"
#include "jsusfx-framework.h"
#include "paobject.h"

#define MIDI_OFF 0x80
#define MIDI_ON 0x90

struct MidiBuffer
{
	static const int kMaxBytes = 1000 * 3;

	uint8_t bytes[kMaxBytes];
	int numBytes = 0;

	bool append(const uint8_t * in_bytes, const int in_numBytes)
	{
		if (numBytes + in_numBytes > MidiBuffer::kMaxBytes)
			return false;

		for (int i = 0; i < in_numBytes; ++i)
			bytes[numBytes++] = in_bytes[i];

		return true;
	}
};

struct MidiKeyboard
{
	static const int kNumKeys = 16;

	struct Key
	{
		bool isDown;
	};

	Key keys[kNumKeys];

	int octave = 4;

	MidiKeyboard()
	{
		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];

			key.isDown = false;
		}
	}

	int getNote(const int keyIndex) const
	{
		return octave * 8 + keyIndex;
	}

	void changeOctave(const int direction)
	{
		octave = octave + direction;
		
		if (octave < 0)
			octave = 0;
		if (octave > 10)
			octave = 10;

		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];

			key.isDown = false;
		}
	}
};

struct FileEditor_JsusFx : FileEditor, PortAudioHandler
{
	JsusFx_Framework jsusFx;

	JsusFxGfx_Framework gfx;
	JsusFxFileAPI_Basic fileApi;
	JsusFxPathLibrary_Basic pathLibary;

	Surface * surface = nullptr;

	bool isValid = false;

	bool firstFrame = true;

	int offsetX = 0;
	int offsetY = 0;
	bool isDragging = false;
	
	bool sliderIsActive[JsusFx::kMaxSliders] = { };
	
	PortAudioObject paObject;
	SDL_mutex * mutex = nullptr;

	MidiBuffer midiBuffer;
	MidiKeyboard midiKeyboard;
	
	bool showMidiKeyboard = true;
	bool showControlSliders = false;

	FileEditor_JsusFx(const char * path);
	virtual ~FileEditor_JsusFx() override;
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override;
	
	virtual void doButtonBar() override;

	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
