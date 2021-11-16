#pragma once

#include "component.h"

#include "Mat4x4.h"

#include <mutex>
#include <vector>

//

namespace ZitaRev1
{
	struct Reverb;
}

//

struct ReverbZoneComponent : Component<ReverbZoneComponent>
{
	Vec3 boxExtents = Vec3(1.f);
	
	float preDelay = .04f;
	float t60Low = 3.f;
	float t60Mid = 2.f;
	float t60CrossoverFrequency = 200.f;
	float dampingFrequency = 6.0e3f;
	float eq1Gain = 0.f;
	float eq2Gain = 0.f;
	
	bool dirty = false;
	
	virtual void drawGizmo(ComponentDraw & draw) const override;
	
	virtual void propertyChanged(void * address) override final
	{
		dirty = true;
	}
};

struct ReverbZoneComponentMgr : ComponentMgr<ReverbZoneComponent>
{
	// the reverb component mgr maintains a copy of the reverb zones accessible exclusively on the audio thread. this allows us to
	// add and remove reverb zone components without blocking the audio thread and vice versa processing the audio thread without
	// blocking the main thread. to manage the copy commands are used to schedule additions, removals and updates on the audio thread
	
	enum CommandType
	{
		kCommandType_None,
		kCommandType_AddZone,
		kCommandType_RemoveZone,
		kCommandType_UpdateZoneParams,    // should be infrequent
		kCommandType_UpdateZoneTransform, // should be infrequent, but could be each frame when parented to an animated node
		kCommandType_UpdateAudioParams
	};
	
	struct Command
	{
		CommandType type = kCommandType_None;
		
		struct
		{
			int id;
		} addZone;
		
		struct
		{
			int id;
		} removeZone;
		
		struct
		{
			int id;
			bool enabled;
			Vec3 boxExtents;
			float preDelay;
			float t60Low;
			float t60Mid;
			float t60CrossoverFrequency;
			float dampingFrequency;
			float eq1Gain;
			float eq2Gain;
		} updateZone;
		
		struct
		{
			int id;
			Mat4x4 worldToObject;
		} updateZoneTransform;
		
		struct
		{
			int frameRate;
			int bufferSize;
		} updateAudioParams;
	};
	
	struct Zone
	{
		int id = -1;
		
		bool enabled = false;
		Vec3 boxExtents = Vec3(1.f);
		
		Mat4x4 worldToObject;
		bool hasTransform = false;
		
		float * inputBuffer = nullptr;
		
		ZitaRev1::Reverb * reverb = nullptr;
	};
	
	std::vector<Zone> zones;
	
	int audioFrameRate = 0;
	int audioBufferSize = 0;
	
	std::vector<Command> commands;
	std::mutex commands_mutex;
	
	bool audioThreadIsActive = false;
	
	//
	
	virtual ReverbZoneComponent * createComponent(const int id) override final;
	virtual void destroyComponent(const int id) override final;
	
	virtual void tickAlways() override final;
	
	void onAudioThreadBegin(const int frameRate, const int bufferSize);
	void onAudioThreadEnd();
	void onAudioThreadProcess();
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
		
		// -- shape properties
		
		add("boxExtents", &ReverbZoneComponent::boxExtents);
		
		// -- reverb properties
		
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
