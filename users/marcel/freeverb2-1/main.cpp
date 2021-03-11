#include "framework.h"

#define SAMPLE_RATE 44100.f

/*
    == SOUL example code ==
    == Author: Jules via the class Freeverb algorithm ==
*/

/// Title: An implementation of the classic Freeverb reverb algorithm.

//==============================================================================
static const float allpassFilter_retainValue = powf(0.5f, 44100 / float(SAMPLE_RATE));

template <int bufferSize>
struct AllpassFilter
{
	float buffer[bufferSize] = { };

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
static const float combFilter_gain = powf(0.015f, 44100 / float(SAMPLE_RATE));

template <int bufferSize>
struct CombFilter
{
    float buffer[bufferSize] = { };

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
		const float audioInDry,
		const float dryIn,
		const float wet1In,
		const float wet2In,
		float & __restrict audioOut1,
		float & __restrict audioOut2)
    {
		const float left  = (audioInLeftWet  * wet1In) + (audioInRightWet * wet2In);
		const float right = (audioInRightWet * wet1In) + (audioInLeftWet  * wet2In);

		audioOut1 = left + audioInDry * dryIn;
		audioOut2 = right + audioInDry * dryIn;
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

struct event
{
	typedef std::function<void(float newValue)> action_t;
	
	action_t action;
	
	event(const action_t & in_action)
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

    event roomSize { [this](float newValue)    { roomSizeScaled = newValue / 100.0f; onUpdate(); } };
    event damping  { [this](float newValue)
		{
			dampingScaled  = newValue / 100.0f;
			dampingScaled = powf(dampingScaled, 44100 / float(SAMPLE_RATE));
			onUpdate();
		} };
    event wetLevel { [this](float newValue)    { wetLevelScaled = newValue / 100.0f; onUpdate(); } };
    event dryLevel { [this](float newValue)    { dryLevelScaled = newValue / 100.0f; onUpdate(); } };
    event width    { [this](float newValue)    { widthScaled    = newValue / 100.0f; onUpdate(); } };

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
	
	AllpassFilter<225 + offset> allpass_1;
	AllpassFilter<341 + offset> allpass_2;
	AllpassFilter<441 + offset> allpass_3;
	AllpassFilter<556 + offset> allpass_4;

	CombFilter<1116 + offset> comb_1;
	CombFilter<1188 + offset> comb_2;
	CombFilter<1277 + offset> comb_3;
	CombFilter<1356 + offset> comb_4;
	CombFilter<1422 + offset> comb_5;
	CombFilter<1491 + offset> comb_6;
	CombFilter<1557 + offset> comb_7;
	CombFilter<1617 + offset> comb_8;

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

/*
		// todo : does this branch out and sum the results individually ? or are all comb results summed instead ?
        comb_1,
        comb_2,
        comb_3,
        comb_4,
        comb_5,
        comb_6,
        comb_7,
        comb_8  -> allpass_1 -> allpass_2 -> allpass_3 -> allpass_4 -> audioOut;
*/

		return audioOut;
    }
};

#include "ui.h"

struct UiBinding
{
	event & event;
	std::string name;
	float min;
	float max;
	float init;
	std::string text;
	std::string unit;
	float step;
	
	float value;
	
	void operator=(const float in_value)
	{
		value = clamp<float>(in_value, min, max);
		
		event = value;
	}
};

static void doUiBinding(UiBinding & binding)
{
	float value = binding.value;
	doSlider(value, binding.min, binding.max, binding.name.c_str(), 0.f, framework.timeStep);
	
	binding = value;
}

struct Reverb
{
	ReverbParameterProcessorParam param;
	
	UiBinding roomSizeUi { .event=param.roomSize, .name="Room Size",      .min=0, .max=100, .init=80,  .text="tiny|small|medium|large|hall" };
	UiBinding dampingUi  { .event=param.damping,  .name="Damping Factor", .min=0, .max=100, .init=50,  .unit="%%", .step=1 };
	UiBinding wetLevelUi { .event=param.wetLevel, .name="Wet Level",      .min=0, .max=100, .init=33,  .unit="%%", .step=1 };
	UiBinding dryLevelUi { .event=param.dryLevel, .name="Dry Level",      .min=0, .max=100, .init=40,  .unit="%%", .step=1 };
	UiBinding widthUi    { .event=param.width,    .name="Width",          .min=0, .max=100, .init=100, .unit="%%", .step=1 };
	
	Reverb()
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

    void process(float audioIn, float & audioOut1, float & audioOut2)
    {
        // Parameter outputs to smoothing processors
		dryGainParameterRamp.updateParameter(param.dryGainOut);
        wetGain1ParameterRamp.updateParameter(param.wetGain1Out);
        wetGain2ParameterRamp.updateParameter(param.wetGain2Out);
        dampingParameterRamp.updateParameter(param.dampingOut);
        feedbackParameterRamp.updateParameter(param.feedbackOut);
        
        // Sum the audio
        // todo : audioIn -> Mixer.audioInDry;

		const float damping = dampingParameterRamp.process();
		const float feedback = feedbackParameterRamp.process();
		
        // Left channel
        const float audioInLeftWet = reverbChannelLeft.process(audioIn, damping, feedback);
        // todo : reverbChannelLeft                  -> Mixer.audioInLeftWet;

        // Right channel
        const float audioInRightWet = reverbChannelRight.process(audioIn, damping, feedback);
        // todo : reverbChannelRight        -> Mixer.audioInRightWet;

		Mixer mixer;
		mixer.process(audioInLeftWet, audioInRightWet, audioIn, dryGainParameterRamp.process(), wetGain1ParameterRamp.process(), wetGain2ParameterRamp.process(), audioOut1, audioOut2);
    }
};

#include "audiooutput/AudioOutput_Native.h"

struct AudioStream_ReverbTest : AudioStream
{
	Reverb reverb;
	
	int noiseSamplesLeft = 0;
	
	virtual int Provide(int numSamples, AudioSample * samples) override
	{
		for (int i = 0; i < numSamples; ++i)
		{
			float audioIn = 0.f;
			
			if (noiseSamplesLeft > 0)
			{
				noiseSamplesLeft--;
				audioIn = random<float>(-.1f, +.1f);
			}
		
			float audioOut1;
			float audioOut2;
			reverb.process(audioIn, audioOut1, audioOut2);
			
			samples[i].channel[0] = audioOut1 * (1 << 15);
			samples[i].channel[1] = audioOut2 * (1 << 15);
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
	uiState.font = "engine/fonts/Roboto-Regular.ttf"; // fixme : don't let UiState define a font by default, or ensure the font exists
	
	AudioOutput_Native audioOutput;
	audioOutput.Initialize(2, 44100, 256);
	
	AudioStream_ReverbTest audioStream;
	audioOutput.Play(&audioStream);
	
	for (;;)
	{
		//framework.waitForEvents = true;
		
		framework.process();
		
		if (framework.quitRequested)
			break;
			
		if (mouse.isDown(BUTTON_LEFT))
		{
			// fixme : unsafe
			
			audioStream.noiseSamplesLeft = SAMPLE_RATE / 20;
		}
		
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
