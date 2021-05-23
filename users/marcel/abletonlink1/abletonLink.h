#pragma once

#include <stdint.h>

/*
Ableton Link wrapper.

This wrapper is mainly in existence due to the fact the official Ableton Link library is a header-only library, that introduces almost four seconds (on my system) of additional compile time to each file where it's included in. This wrapper includes a minimum of header files to keep compilation speedy.
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
		
		double beatAtTime(const uint64_t timeUs, const double quantum) const;
		double phaseAtTime(const uint64_t timeUs, const double quantum) const;
		
		bool isPlaying() const;
		double tempo() const;
		
		void setIsPlayingAtTime(const bool isPlaying, const uint64_t timeUs);
		void setTempoAtTime(const double tempo, const uint64_t timeUs);
		
		void commitApp() const;
		void commitAudio() const;
	};
	
	// Ableton Link object. If you want to call methods on it directly, you'll have to include <ableton/Link.hpp> first.
	
	ableton::Link * link = nullptr;
	bool ownsLink = false;
	
	// Creation and destruction of the Ableton Link object.

	bool init(const double bpm);
	bool init(ableton::Link * link);
	void shut();

	// Ableton Link interface.
	
	void enable(const bool enable);
	void enableStartStopSync(const bool enable);

	bool isEnabled() const;
	
	int numPeers() const;

	uint64_t getClockTimeUs() const;
	
	SessionState captureAppSessionState() const;
	SessionState captureAudioSessionState() const;
};
