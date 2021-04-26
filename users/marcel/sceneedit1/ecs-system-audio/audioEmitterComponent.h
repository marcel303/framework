#pragma once

#include "component.h"

struct AudioEmitterComponent : Component<AudioEmitterComponent>
{
};

struct AudioEmitterComponentMgr : ComponentMgr<AudioEmitterComponent>
{
};

extern AudioEmitterComponentMgr g_audioEmitterComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct AudioEmitterComponentType : ComponentType<AudioEmitterComponent>
{
	AudioEmitterComponentType()
		: ComponentType("AudioEmitterComponent", &g_audioEmitterComponentMgr)
	{
	}
};

#endif
