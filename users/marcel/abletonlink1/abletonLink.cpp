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

double AbletonLink::SessionState::beatAtTick(const uint64_t tick, const double quantum) const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	const auto micros = link->clock().ticksToMicros(tick);
	
	return sessionState->beatAtTime(micros, quantum);
}

double AbletonLink::SessionState::phaseAtTick(const uint64_t tick, const double quantum) const
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	const auto micros = link->clock().ticksToMicros(tick);
	
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

void AbletonLink::SessionState::setIsPlayingAtTick(const bool isPlaying, const uint64_t tick)
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	sessionState->setIsPlaying(isPlaying, link->clock().ticksToMicros(tick));
}

void AbletonLink::SessionState::setTempoAtTick(const double tempo, const uint64_t tick)
{
	ableton::Link::SessionState * sessionState = (ableton::Link::SessionState*)data;
	
	sessionState->setTempo(tempo, link->clock().ticksToMicros(tick));
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

	auto s = link->captureAppSessionState();
	printf("tempo: %.2f\n", s.tempo());
	
	return true;
}

void AbletonLink::shut()
{
	delete link;
	link = nullptr;
}

void AbletonLink::enable(const bool enable)
{
	link->enable(enable);
	
	auto s = link->captureAppSessionState();
	printf("tempo: %.2f\n", s.tempo());
}

void AbletonLink::enableStartStopSync(const bool enable)
{
	link->enableStartStopSync(enable);
	
	auto s = link->captureAppSessionState();
	printf("tempo: %.2f\n", s.tempo());
}

bool AbletonLink::isEnabled() const
{
	return link->isEnabled();
}

uint64_t AbletonLink::getClockTick() const
{
	return link->clock().ticks();
}

AbletonLink::SessionState AbletonLink::captureAppSessionState() const
{
	auto s = link->captureAppSessionState();
	printf("tempo: %.2f\n", s.tempo());
	
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
