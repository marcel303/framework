#ifndef RESSND_H
#define RESSND_H
#pragma once

#include "Res.h"

#define WITH_SDL_MIXER 1 // fixme

struct Mix_Chunk;

class ResSnd : public Res
{
public:
	ResSnd();

	void operator=(Mix_Chunk* data);

//FIXME private:
	Mix_Chunk* m_data;
};

#endif
