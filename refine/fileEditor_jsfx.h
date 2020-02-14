#pragma once

#include "fileEditor.h"
#include "gfx-framework.h"
#include "jsusfx_file.h"
#include "jsusfx-framework.h"
#include "paobject.h"
#include "Random.h"
#include <atomic>

class RtMidiIn;

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

struct JsusFxWindow
{
	static const int kBorderSize = 6;
	static const int kTitlebarSize = 30;

	enum State
	{
		kState_Idle,
		kState_DragMove,
		kState_DragSize
	};

	State state = kState_Idle;
	bool isFocused = false;
	
	int x = 0;
	int y = 0;
	int clientSx = 0;
	int clientSy = 0;
	const char * caption = nullptr;
	
	bool isVisible = true;

	void init(const int x, const int y, const int clientSx, const int clientSy, const char * caption);

	void tick(const float dt, bool & inputIsCaptured);
	void drawDecoration() const;

	void getClientRect(int & x, int & y, int & sx, int & sy) const;
};

struct FileEditor_JsusFx : FileEditor, PortAudioHandler
{
	enum AudioSource
	{
		// note : must update doButtonBar and reflect when this is changed !
		kAudioSource_Silence,
		kAudioSource_PinkNoise,
		kAudioSource_WhiteNoise,
		kAudioSource_Sine,
		kAudioSource_Tent,
		kAudioSource_AudioInterface,
		kAudioSource_Sample // todo : add sample-based audio source
	};
	
	JsusFx_Framework jsusFx;

	JsusFxGfx_Framework gfx;
	JsusFxFileAPI_Basic fileApi;
	JsusFxPathLibrary_Basic pathLibary;

	Surface * surface = nullptr;

	bool isValid = false;

	bool firstFrame = true;

	int offsetX = 0;
	int offsetY = 0;
	
	bool sliderIsActive[JsusFx::kMaxSliders] = { };

	// midi input

	RtMidiIn * midiIn = nullptr;
	int currentMidiPort = -1;
	int desiredMidiPort = 0;
	
	// audio output

	PortAudioObject paObject;
	SDL_mutex * mutex = nullptr;

	// source audio synthesis

	AudioSource audioSource = kAudioSource_PinkNoise;
	int volume = 25;
	int frequency = 440;
	int sharpness = 0;
	
	struct Synthesis
	{
		Synthesis()
			: audioSource(kAudioSource_Silence)
			, volume(25)
			, frequency(440)
			, sharpness(0)
			, pinkNumber(1 << 16)
		{
		}
		
		std::atomic<AudioSource> audioSource;
		std::atomic<int> volume;
		std::atomic<int> frequency;
		std::atomic<int> sharpness;
		RNG::PinkNumber pinkNumber;
		float sinePhase = 0.f;
		float tentPhase = 0.f;
	} synthesis;

	MidiBuffer midiBuffer;
	MidiKeyboard midiKeyboard;

	JsusFxWindow midiKeyboardWindow;
	JsusFxWindow jsusFxWindow;
	JsusFxWindow controlSlidersWindow;

	FileEditor_JsusFx(const char * path);
	virtual ~FileEditor_JsusFx() override;
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int numOutputChannels,
		const int framesPerBuffer) override;
	
	virtual void doButtonBar() override;

	void updateMidi();
	
	void updateSynthesisParams();

	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
