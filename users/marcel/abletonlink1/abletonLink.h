#pragma once

#include <stdint.h>

/*
Ableton Link wrapper.

This wrapper is mainly in existence due to the fact the official Ableton Link library is a header-only library, that introduces almost four seconds (on my system) of additional compile time to each file where its included in. This wrapper includes a minimum of header files to keep compilation speedy.
*/

// forward declaration of the Ableton Link object

namespace ableton
{
	class Link;
}

struct AbletonLink
{
	struct SessionState
	{
		friend AbletonLink;
		
	private:
		uint64_t data[128/8];
		
		ableton::Link * link = nullptr;
		
	public:
		SessionState();
		~SessionState();
		
		double beatAtTick(const uint64_t tick, const double quantum) const;
		double phaseAtTick(const uint64_t tick, const double quantum) const;
		
		bool isPlaying() const;
		double tempo() const;
		
		void setIsPlayingAtTick(const bool isPlaying, const uint64_t tick);
		void setTempoAtTick(const double tempo, const uint64_t tick);
		
		void commitApp() const;
		void commitAudio() const;
	};
	
	// Ableton Link object. If you want to calls methods on it directly, you'll have to include <ableton/Link.hpp> first.
	
	ableton::Link * link = nullptr;
	
	// Creation and destruction of the Ableton Link object.

	bool init(const double bpm);
	void shut();

	// Ableton Link interface.
	
	void enable(const bool enable);
	void enableStartStopSync(const bool enable);

	bool isEnabled() const;

	uint64_t getClockTick() const;
	
	SessionState captureAppSessionState() const;
	SessionState captureAudioSessionState() const;
};
