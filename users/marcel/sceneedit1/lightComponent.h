#pragma once

#include "component.h"

class Surface;

struct LightComponent : Component<LightComponent>
{
};

struct LightComponentMgr : ComponentMgr<LightComponent>
{
	Surface * lightAccumulationSurface = nullptr;
	
	virtual bool init() override;
	virtual void shut() override;
	
	void drawDeferredLights() const;
	void drawDeferredLightApplication(const int colorTextureId) const;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct LightComponentType : ComponentType<LightComponent>
{
	LightComponentType()
		: ComponentType("LightComponent")
	{
		tickPriority = kComponentPriority_Light;
	}
};

#endif
