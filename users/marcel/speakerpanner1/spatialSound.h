#pragma once

#include "panner.h"

class AudioSource;

struct SpatialSound
{
	AudioSource * audioSource = nullptr;
	
	SpeakerPanning::Source panningSource;
};
