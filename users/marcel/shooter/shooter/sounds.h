#pragma once

enum Sound
{
	Sound_Begin1,
	Sound_Bubble1,
	Sound_Missile1,
	Sound_Missile2,
	Sound_Swarm1,
	Sound_Swarm2,
	Sound_Vulcan1,
	Sound_Vulcan2,
	Sound_Shockwave1,
	Sound_Shockwave2,
	Sound_Kill1,
	Sound_Damage1,
	Sound_Damage2,
	Sound_End1
};

void SoundInit();
void SoundShutdown();
void SoundPlay(Sound sound);
