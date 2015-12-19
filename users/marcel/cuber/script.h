#pragma once

#include <SDL2/SDL.h>

class EffectCtx;
class Effect;

typedef Effect * (__cdecl * CreateFunction)(EffectCtx & ctx);
typedef void (__cdecl * DestroyFunction)(Effect * effect);
