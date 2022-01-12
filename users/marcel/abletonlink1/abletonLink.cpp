#include "ableton/Link.hpp"
#include "abletonLink.h"

AbletonLink::SessionState::SessionState()
{
	memset(data, 0, sizeof(data));
}

AbletonLink::SessionState::~SessionState()
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	sessionState->~SessionState();
}

double AbletonLink::SessionState::beatAtTime(const uint64_t timeUs, const double quantum) const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	const auto micros = std::chrono::microseconds(timeUs);
	
	return sessionState->beatAtTime(micros, quantum);
}

double AbletonLink::SessionState::phaseAtTime(const uint64_t timeUs, const double quantum) const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	const auto micros = std::chrono::microseconds(timeUs);
	
	return sessionState->phaseAtTime(micros, quantum);
}

bool AbletonLink::SessionState::isPlaying() const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	return sessionState->isPlaying();
}

double AbletonLink::SessionState::tempo() const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	return sessionState->tempo();
}

void AbletonLink::SessionState::setIsPlayingAtTime(const bool isPlaying, const uint64_t timeUs)
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	const auto micros = std::chrono::microseconds(timeUs);
	
	sessionState->setIsPlaying(isPlaying, micros);
}

void AbletonLink::SessionState::setTempoAtTime(const double tempo, const uint64_t timeUs)
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	const auto micros = std::chrono::microseconds(timeUs);
	
	sessionState->setTempo(tempo, micros);
}

void AbletonLink::SessionState::commitApp() const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	link->commitAppSessionState(*sessionState);
}

void AbletonLink::SessionState::commitAudio() const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	link->commitAudioSessionState(*sessionState);
}

//

bool AbletonLink::init(const double bpm)
{
	link = new ableton::Link(bpm);
	ownsLink = true;
	
	return true;
}

bool AbletonLink::init(ableton::Link * in_link)
{
	link = in_link;
	ownsLink = false;
	
	return true;
}

void AbletonLink::shut()
{
	if (ownsLink)
		delete link;
	link = nullptr;
	ownsLink = false;
}

void AbletonLink::enable(const bool enable)
{
	link->enable(enable);
}

void AbletonLink::enableStartStopSync(const bool enable)
{
	link->enableStartStopSync(enable);
}

bool AbletonLink::isEnabled() const
{
	return link->isEnabled();
}

bool AbletonLink::isStartStopSyncEnabled() const
{
	return link->isStartStopSyncEnabled();
}

int AbletonLink::numPeers() const
{
	return (int)link->numPeers();
}

uint64_t AbletonLink::getClockTimeUs() const
{
	return link->clock().micros().count();
}

void AbletonLink::setTempoCallback(const std::function<void()> & callback)
{
	link->setTempoCallback([this](const double bpm)
		{
			if (tempoCallback != nullptr)
				tempoCallback();
		});
	
	tempoCallback = callback;
}

AbletonLink::SessionState AbletonLink::captureAppSessionState() const
{
	SessionState result;
	
	assert(sizeof(result.data) >= sizeof(ableton::Link::SessionState));
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)result.data;
	
	*sessionState = link->captureAppSessionState();
	
	result.link = link;
	
	return result;
}

AbletonLink::SessionState AbletonLink::captureAudioSessionState() const
{
	SessionState result;
	
	assert(sizeof(result.data) >= sizeof(ableton::Link::SessionState));
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)result.data;
	
	*sessionState = link->captureAudioSessionState();
	
	result.link = link;
	
	return result;
}
