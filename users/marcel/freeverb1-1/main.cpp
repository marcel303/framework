#include "framework.h"

#include <string.h> // memset

#define SAMPLE_RATE 44100
//#define SAMPLE_RATE 48000
//#define SAMPLE_RATE 96000
//#define SAMPLE_RATE 192000

#define BaseSampleRate 44100 // freeverb constants assume 44.1kHz
#define ScaleBufferSize(bufferSize) ((bufferSize) * SAMPLE_RATE / BaseSampleRate)

/*
    == SOUL example code ==
    == Author: Jules via the class Freeverb algorithm ==
*/

/// Title: An implementation of the classic Freeverb reverb algorithm.

//==============================================================================
static const float allpassFilter_retainValue = 0.5f; // default Freeverb
//static const float allpassFilter_retainValue = 0.618f; // golden ration, true allpass

// https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html#14451:
/* As discussed above in §3.4.2, the allpass filters provide ``colorless'' high-density echoes in the late impulse response of the reverberator [420,421]. These allpass filters may also be referred to as diffusers. While allpass filters are ``colorless'' in theory, perceptually, their impulse responses are only colorless when they are extremely short (less than 10 ms or so). Longer allpass impulse responses sound similar to feedback comb-filters. For steady-state tones, however, such as sinusoids, the allpass property gives the same gain at every frequency, unlike comb filters. */
struct AllpassFilter
{
	AllpassFilter(const int in_bufferSize)
		: buffer(new float[ScaleBufferSize(in_bufferSize)])
		, bufferSize(ScaleBufferSize(in_bufferSize))
	{
		memset(buffer, 0, bufferSize * sizeof(buffer[0]));
	}
	
	~AllpassFilter()
	{
		delete [] buffer;
		buffer = nullptr;
	}
	
	float * buffer = nullptr;
	int bufferSize = 0;

	int bufferIndex = 0;
	
    float process(const float audioIn)
    {
		const float bufferedValue = buffer[bufferIndex];

		buffer[bufferIndex] = audioIn + (bufferedValue * allpassFilter_retainValue);

		bufferIndex++;
		if (bufferIndex == bufferSize)
			bufferIndex = 0;
		
		return bufferedValue - audioIn;
    }
};

//==============================================================================
static const float combFilter_gain = 0.015f;

// https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html#14451:
/* The parallel comb-filter bank is intended to give a psychoacoustically appropriate fluctuation in the reverberator frequency response. As discussed in Chapter 2 (§2.6.2), a feedback comb filter can simulate a pair of parallel walls, so one could choose the delay-line length in each comb filter to be the number of samples it takes for a plane wave to propagate from one wall to the opposite wall and back. However, in his original paper [415], Schroeder describes a more psychoacoustically motivated approach: */
struct CombFilter
{
	// note : 'room size' should modulate the buffer size here to be properly called
	//        'room size', otherwise 'room size' is more like a 'liveness' or 'reflectivity' factor
	
	CombFilter(const int in_bufferSize)
		: buffer(new float[ScaleBufferSize(in_bufferSize)])
		, bufferSize(ScaleBufferSize(in_bufferSize))
	{
		memset(buffer, 0, bufferSize * sizeof(buffer[0]));
	}
	
	~CombFilter()
	{
		delete [] buffer;
		buffer = nullptr;
	}
	
    float * buffer = nullptr;
    int bufferSize = 0;

	int bufferIndex = 0;
	
	float gain = combFilter_gain;
	float last = 0.0f;
        
    float process(
		const float audioIn,
		const float dampingIn,
		const float feedbackLevelIn)
    {
		const float out = buffer[bufferIndex];

		last = (out * (1.0f - dampingIn)) + (last * dampingIn);

		buffer[bufferIndex] = (gain * audioIn) + (last * feedbackLevelIn);
		
		bufferIndex++;
		if (bufferIndex == bufferSize)
			bufferIndex = 0;

		return out;
    }
};

//==============================================================================
struct Mixer
{
    void process(
		const float audioInLeftWet,
		const float audioInRightWet,
		const float audioInDry1,
		const float audioInDry2,
		const float dryIn,
		const float wet1In,
		const float wet2In,
		float & __restrict audioOut1,
		float & __restrict audioOut2)
    {
		const float left  = (audioInLeftWet  * wet1In) + (audioInRightWet * wet2In);
		const float right = (audioInRightWet * wet1In) + (audioInLeftWet  * wet2In);

		audioOut1 = left + audioInDry1 * dryIn;
		audioOut2 = right + audioInDry2 * dryIn;
    }
};

//==============================================================================
// Converts an input value into a stream (limited to the given slewRate)
template <int slewRateMs>
struct ParameterRamp
{
	static constexpr float slewRate = slewRateMs / 1000.f;
	
    void updateParameter(
		const float newTarget)
    {
        targetValue = newTarget;

        const float diff = targetValue - currentValue;
        const float rampSeconds = abs (diff) / slewRate;

        rampSamples   = int(SAMPLE_RATE * rampSeconds);
        rampIncrement = diff / float(rampSamples);
    }

    float targetValue;
    float currentValue;
    float rampIncrement;
    int rampSamples;

    float process()
    {
		if (rampSamples > 0)
		{
			currentValue += rampIncrement;
			--rampSamples;
			
			if (rampSamples == 0)
				currentValue = targetValue;
		}
		
		return currentValue;
    }
};

#if 1

#include <functional>

struct UiEvent
{
	typedef std::function<void(float newValue)> action_t;
	
	action_t action;
	
	UiEvent(const action_t & in_action)
		: action(in_action)
	{
	}
	
	void operator=(const float newValue)
	{
		// todo : schedule assignment on the audio thread
		action(newValue);
	}
};

//==============================================================================
// Correctly applies parameter changes to the streams of input to the algorithm
struct ReverbParameterProcessorParam
{
    float
		dryGainOut,
		wetGain1Out,
		wetGain2Out,
		dampingOut,
		feedbackOut;

    UiEvent roomSize { [this](float newValue)    { roomSizeScaled = newValue / 100.0f; onUpdate(); } };
    UiEvent damping  { [this](float newValue)
		{
			dampingScaled  = newValue / 100.0f;
			dampingScaled = powf(dampingScaled, BaseSampleRate / float(SAMPLE_RATE));
			onUpdate();
		} };
    UiEvent wetLevel { [this](float newValue)    { wetLevelScaled = newValue / 100.0f; onUpdate(); } };
    UiEvent dryLevel { [this](float newValue)    { dryLevelScaled = newValue / 100.0f; onUpdate(); } };
    UiEvent width    { [this](float newValue)    { widthScaled    = newValue / 100.0f; onUpdate(); } };

    float roomSizeScaled = 0.5f;
    float dampingScaled  = 0.5f;
    float wetLevelScaled = 0.33f;
    float dryLevelScaled = 0.4f;
    float widthScaled    = 1.0f;

    void onUpdate()
    {
        // Various tuning factors for the reverb
        constexpr float wetScaleFactor  = 3.0f;
        constexpr float dryScaleFactor  = 2.0f;

        constexpr float roomScaleFactor = 0.28f;
        constexpr float roomOffset      = 0.7f;
        constexpr float dampScaleFactor = 0.4f;

        // Write updated values
        dryGainOut  = dryLevelScaled * dryScaleFactor;
        wetGain1Out = 0.5f * wetLevelScaled * wetScaleFactor * (1.0f + widthScaled);
        wetGain2Out = 0.5f * wetLevelScaled * wetScaleFactor * (1.0f - widthScaled);
        dampingOut  = dampingScaled * dampScaleFactor;
        feedbackOut = roomSizeScaled * roomScaleFactor + roomOffset;
    }
};

#endif

//==============================================================================
// Mono freeverb implementation
template <int offset>
struct ReverbChannel
{
	// see: https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
	// for the magical offset constants used
	
	AllpassFilter allpass_1 { 225 + offset };
	AllpassFilter allpass_2 { 341 + offset };
	AllpassFilter allpass_3 { 441 + offset };
	AllpassFilter allpass_4 { 556 + offset };

	CombFilter comb_1 { 1116 + offset };
	CombFilter comb_2 { 1188 + offset };
	CombFilter comb_3 { 1277 + offset };
	CombFilter comb_4 { 1356 + offset };
	CombFilter comb_5 { 1422 + offset };
	CombFilter comb_6 { 1491 + offset };
	CombFilter comb_7 { 1557 + offset };
	CombFilter comb_8 { 1617 + offset };

    float process(
		const float audioIn,
		const float damping,
		const float feedback)
    {
		const float audioOut1 = comb_1.process(audioIn, damping, feedback);
		const float audioOut2 = comb_2.process(audioIn, damping, feedback);
		const float audioOut3 = comb_3.process(audioIn, damping, feedback);
		const float audioOut4 = comb_4.process(audioIn, damping, feedback);
		const float audioOut5 = comb_5.process(audioIn, damping, feedback);
		const float audioOut6 = comb_6.process(audioIn, damping, feedback);
		const float audioOut7 = comb_7.process(audioIn, damping, feedback);
		const float audioOut8 = comb_8.process(audioIn, damping, feedback);
		
		float audioOut =
			((audioOut1 + audioOut2) + (audioOut3 + audioOut4)) +
			((audioOut5 + audioOut6) + (audioOut7 + audioOut8));
		
		audioOut = allpass_1.process(audioOut);
		audioOut = allpass_2.process(audioOut);
		audioOut = allpass_3.process(audioOut);
		audioOut = allpass_4.process(audioOut);

		return audioOut;
    }
};

#include "ui.h"

struct UiBinding
{
	UiEvent & event;
	const char * name;
	float min;
	float max;
	float init;
	float value;
	
	UiBinding(UiEvent & in_event, const char * in_name, const float in_min, const float in_max, const float in_init)
		: event(in_event)
		, name(in_name)
		, min(in_min)
		, max(in_max)
		, init(in_init)
		, value(in_init)
	{
	}
	
	void operator=(const float in_value)
	{
		value = clamp<float>(in_value, min, max);
		
		event = value;
	}
	
	const char * name_get() const
	{
		return name == nullptr ? "" : name;
	}
};

static void doUiBinding(UiBinding & binding)
{
	float value = binding.value;
	doSlider(value, binding.min, binding.max, binding.name_get(), 0.f, framework.timeStep);
	
	binding = value;
}

struct Reverb
{
	ReverbParameterProcessorParam param;
	
	UiBinding roomSizeUi;
	UiBinding dampingUi;
	UiBinding wetLevelUi;
	UiBinding dryLevelUi;
	UiBinding widthUi;
	
	Reverb()
		: roomSizeUi (param.roomSize, "Room Size",      0, 100, 80  )
		, dampingUi  (param.damping,  "Damping Factor", 0, 100, 50  )
		, wetLevelUi (param.wetLevel, "Wet Level",      0, 100, 33  )
		, dryLevelUi (param.dryLevel, "Dry Level",      0, 100, 40  )
		, widthUi    (param.width,    "Width",          0, 100, 100 )
	{
		roomSizeUi = 80;
		dampingUi  = 50;
		wetLevelUi = 33;
		dryLevelUi = 40;
		widthUi    = 100;
	}
	
	void doUi()
	{
		doUiBinding(roomSizeUi);
		doUiBinding(dampingUi);
		doUiBinding(wetLevelUi);
		doUiBinding(dryLevelUi);
		doUiBinding(widthUi);
	}
	
	ParameterRamp <20000> dryGainParameterRamp;
	ParameterRamp <20000> wetGain1ParameterRamp;
	ParameterRamp <20000> wetGain2ParameterRamp;
	ParameterRamp <20000> dampingParameterRamp;
	ParameterRamp <20000> feedbackParameterRamp;

	ReverbChannel <0> reverbChannelLeft;
	ReverbChannel <23> reverbChannelRight;

    void process(
		const float audioIn1,
		const float audioIn2,
		float & audioOut1,
		float & audioOut2)
    {
		// Parameter outputs to smoothing processors
		dryGainParameterRamp.updateParameter(param.dryGainOut);
        wetGain1ParameterRamp.updateParameter(param.wetGain1Out);
        wetGain2ParameterRamp.updateParameter(param.wetGain2Out);
        dampingParameterRamp.updateParameter(param.dampingOut);
        feedbackParameterRamp.updateParameter(param.feedbackOut);

		const float damping = dampingParameterRamp.process();
		const float feedback = feedbackParameterRamp.process();
		
		const float audioIn = (audioIn1 + audioIn2) * 0.5f;
		
        // Left channel
        const float audioInLeftWet = reverbChannelLeft.process(audioIn, damping, feedback);

        // Right channel
        const float audioInRightWet = reverbChannelRight.process(audioIn, damping, feedback);

		Mixer mixer;
		mixer.process(
			audioInLeftWet,
			audioInRightWet,
			audioIn1,
			audioIn2,
			dryGainParameterRamp.process(),
			wetGain1ParameterRamp.process(),
			wetGain2ParameterRamp.process(),
			audioOut1,
			audioOut2);
    }
};

#include "audiooutput/AudioOutput_Native.h"
#include "audiostream/AudioStreamWave.h"

struct AudioStream_ReverbTest : AudioStream
{
	AudioStreamWave source;
	
	Reverb reverb;
	
	int noiseSamplesLeft = 0;
	double noisePhase = 0.0f;
	
	AudioStream_ReverbTest()
	{
		source.Open("snare.wav", true);
	}
	
	virtual int Provide(int numSamples, AudioSample * samples) override
	{
		const int numSourceSamples = source.Provide(numSamples, samples);
		
		for (int i = 0; i < numSamples; ++i)
		{
			float audioIn1 = 0.f;
			float audioIn2 = 0.f;
			
			if (noiseSamplesLeft > 0)
			{
				noiseSamplesLeft--;
			#if 1
				noisePhase = fmod(noisePhase + 440.0 / SAMPLE_RATE, 1.0);
				audioIn1 = (noisePhase - 0.5) * 2.0 * 0.1 * float(1 << 15);
				audioIn2 = (noisePhase - 0.5) * 2.0 * 0.1 * float(1 << 15);
			#else
				audioIn1 = random<float>(-.1f, +.1f) * float(1 << 15);
				audioIn2 = random<float>(-.1f, +.1f) * float(1 << 15);
			#endif
			}
			
			if (i < numSourceSamples)
			{
				const float gain = .25f; 
				audioIn1 += float(samples[i].channel[0]) * gain;
				audioIn2 += float(samples[i].channel[1]) * gain;
			}
			
			float audioOut1;
			float audioOut2;
			reverb.process(audioIn1, audioIn2, audioOut1, audioOut2);
			
			samples[i].channel[0] = int16_t(audioOut1);
			samples[i].channel[1] = int16_t(audioOut2);
		}
		
		return numSamples;
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.init(640, 480);
	
	initUi();
	UiState uiState;
	uiState.font = "engine/fonts/Roboto-Regular.ttf"; // fixme : don't let UiState define a font by default, or ensure the font exists. or add calibri.ttf as a resource to libparticle-ui
	uiState.x = 100;
	uiState.y = 100;
	uiState.sx = 400;
	
	AudioOutput_Native audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, 256);
	
	AudioStream_ReverbTest audioStream;
	audioOutput.Play(&audioStream);
	
	for (;;)
	{
		//framework.waitForEvents = true;
		
		framework.process();
		
		if (framework.quitRequested)
			break;
			
	#if 0
		if (mouse.isDown(BUTTON_LEFT))
		{
			// fixme : unsafe
			
			audioStream.noiseSamplesLeft = SAMPLE_RATE / 20;
		}
	#endif
		
		framework.beginDraw(0, 0, 0, 0);
		{
			makeActive(&uiState, true, true);
			pushMenu("Reverb");
			{
				audioStream.reverb.doUi();
			}
			popMenu();
		}
		framework.endDraw();
	}
	
	audioOutput.Shutdown();
	
	shutUi();
	
	framework.shutdown();
	
	return 0;
}
