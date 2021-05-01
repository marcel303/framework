#pragma once

#include "component.h"
#include <mutex>

#include "reverb.h" // todo : rename to reverb-zita-rev1.h

struct ReverbZoneComponent : Component<ReverbZoneComponent>
{
	float preDelay = .04f;
	float t60Low = 3.f;
	float t60Mid = 2.f;
	float t60CrossoverFrequency = 200.f;
	float dampingFrequency = 6.0e3f;
	float eq1Gain = 0.f;
	float eq2Gain = 0.f;
		
	//
	
	float inputBuffer[2][256]; // todo : max buffer size
	
	ZitaRev1::Reverb reverb;
	
	virtual bool init() override final
	{
	// todo : configure audio frame rate somewhere and react to changes
		reverb.init(48000, false);
		reverb.set_opmix(1.f);
		reverb.set_rgxyz(0.f);
		
		return true;
	}
};

struct ReverbZoneComponentMgr : ComponentMgr<ReverbZoneComponent>
{
	std::mutex mutex;
	
	virtual ReverbZoneComponent * createComponent(const int id) override final
	{
		mutex.lock();
		
		auto * component = ComponentMgr<ReverbZoneComponent>::createComponent(id);
		
		mutex.unlock();
		
		// todo : register for audio processing
		
		return component;
	}
	
	virtual void destroyComponent(const int id) override final
	{
		// todo : unregister from audio processing
	
		mutex.lock();
		
		ComponentMgr<ReverbZoneComponent>::destroyComponent(id);
		
		mutex.unlock();
	}
	
	virtual void tick(const float dt) override final
	{
		// todo : update audio processing properties
	}
};

extern ReverbZoneComponentMgr g_reverbZoneComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ReverbZoneComponentType : ComponentType<ReverbZoneComponent>
{
	ReverbZoneComponentType()
		: ComponentType("ReverbZoneComponent", &g_reverbZoneComponentMgr)
	{
		const char * t60Text = "The T60 decay time is the time (in seconds) for the reverberated sound to reach a -60dB attenuation level. The T60 decay time simulates the energy loss as the sound waves bounce against the walls of the reverberation space and interact with air molecules. See the T60 crossover frequency for the crossover between low and mid frequencies.";
		
		add("preDelay", &ReverbZoneComponent::preDelay).limits(.02f, .1f)
			.description(
				"Pre-Delay Time (seconds)",
				"The delay (in seconds) to add to incoming audio signals, before the signals are fed into the reverberation filter. The pre-delay may be thought of as the time it takes an audio signal to cover the distance between an audio emitter and one of walls of the reverberation space.");
		add("t60Low", &ReverbZoneComponent::t60Low).limits(1.f, 8.f)
			.description(
				"T60 Decay Time (seconds) for Low Frequencies",
				t60Text);
		add("t60Mid", &ReverbZoneComponent::t60Mid).limits(1.f, 8.f)
			.description(
				"T60 Decay Time (seconds) for Mid Frequencies",
				t60Text);
		add("t60CrossoverFrequency", &ReverbZoneComponent::t60CrossoverFrequency).limits(50.f, 1000.f)
			.description(
				"T60 Crossover Frequency (Hz)",
				"The frequency below and above which audio signals are filtered by the low or mid T60 filters. Low and mid frequency audio signals are sent to separate T60 attenuation filters, allowing one to set different parameters for each frequency band.");
		add("dampingFrequency", &ReverbZoneComponent::dampingFrequency).limits(1.5e3f, 24.0e3f)
			.description(
				"Damping Frequency (Hz)",
				"The frequency above which audio signals are more aggressively filtered. In the real world, air as a medium for the transmission of sound waves affects sound as if it were a low pass filter, attenuating higher frequencies more than it does lower frequencies. The result of this is that lower frequency sounds travel larger distances. The damping filter models this effect.");
		add("eq1Gain", &ReverbZoneComponent::eq1Gain).limits(-15.f, +15.f)
			.description(
				"Gain for Parametric Equalizer 1",
				"Parametric Equalizer 1 affects sound in the low frequency band, and may be used to boost sound levels to ones taste.");
		add("eq2Gain", &ReverbZoneComponent::eq2Gain).limits(-15.f, +15.f)
			.description(
				"Gain for Parametric Equalizer 2",
				"Parametric Equalizer 2 affects sound in the mid frequency band, and may be used to boost sound levels to ones taste.");
	}
};

#endif
