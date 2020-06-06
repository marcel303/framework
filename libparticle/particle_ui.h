#pragma once

#include "ui.h"

struct ParticleColor;
struct ParticleColorCurve;
struct ParticleCurve;

void doParticleCurve(ParticleCurve & curve, const char * name);
void doParticleColor(ParticleColor & color, const char * name);
void doParticleColorCurve(ParticleColorCurve & curve, const char * name);

void doColorWheel(ParticleColor & color, const char * name, const float dt);
