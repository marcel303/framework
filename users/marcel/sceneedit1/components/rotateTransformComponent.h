#pragma once

#include "component.h"

struct RotateTransformComponent : Component<RotateTransformComponent>
{
	float speed = 0.f;
	
	void tick(const float dt);
};

struct RotateTransformComponentMgr : ComponentMgr<RotateTransformComponent>
{
	virtual void tick(const float dt) override final;
};

extern RotateTransformComponentMgr g_rotateTransformComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct RotateTransformComponentType : ComponentType<RotateTransformComponent>
{
	RotateTransformComponentType()
		: ComponentType("RotateTransformComponent", &g_rotateTransformComponentMgr)
	{
		tickPriority = kComponentPriority_Transform - 1;
		
		add("speed", &RotateTransformComponent::speed);
	}
};

#endif
