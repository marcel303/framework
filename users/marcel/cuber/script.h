#pragma once

class EffectCtx;
class Effect;

#if defined(WIN32)

typedef Effect * (__cdecl * CreateFunction)(EffectCtx & ctx);
typedef void (__cdecl * DestroyFunction)(Effect * effect);

#else

typedef Effect * (* CreateFunction)(EffectCtx & ctx);
typedef void (* DestroyFunction)(Effect * effect);

#endif
