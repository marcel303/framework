#pragma once

#include "component.h"

struct ReverbZoneComponent : Component<ReverbZoneComponent>
{
};

struct ReverbZoneComponentMgr : ComponentMgr<ReverbZoneComponent>
{
};

extern ReverbZoneComponentMgr g_reverbZoneComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ReverbZoneComponentType : ComponentType<ReverbZoneComponent>
{
	ReverbZoneComponentType()
		: ComponentType("ReverbZoneComponent", &g_reverbZoneComponentMgr)
	{
	}
};

#endif
