#pragma once

#include "panner.h"

struct AudioSource;

struct SpatialSound
{
	AudioSource * audioSource = nullptr;
	
	SpeakerPanning::Source panningSource;
};
