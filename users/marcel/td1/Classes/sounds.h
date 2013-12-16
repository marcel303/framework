#pragma once

enum Sound
{
	Sound_Vulcan_Fire,
	Sound_Missile_Fire,
	Sound_Enemy_Die
};

void SoundInit();
void SoundShutdown();
void SoundPlay(Sound sound);
