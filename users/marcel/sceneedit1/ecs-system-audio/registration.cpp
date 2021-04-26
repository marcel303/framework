#include "componentTypeDB.h"

#define DEFINE_COMPONENT_TYPES
#include "audioEmitterComponent.h"
#include "reverbZoneComponent.h"

ComponentTypeRegistration(AudioEmitterComponentType);
ComponentTypeRegistration(ReverbZoneComponentType);

void link_ecssystem_audio() { }
