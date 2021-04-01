#pragma once

#include "component.h"

struct RotateTransformComponent : Component<RotateTransformComponent>
{
	float speed = 0.f;
	
	virtual void tick(const float dt) override final;
};

struct RotateTransformComponentMgr : ComponentMgr<RotateTransformComponent>
{
};

extern RotateTransformComponentMgr g_rotateTransformComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct RotateTransformComponentType : ComponentType<RotateTransformComponent>
{
	RotateTransformComponentType()
		: ComponentType("RotateTransformComponent", &g_rotateTransformComponentMgr)
	{
		add("speed", &RotateTransformComponent::speed);
	}
};

#endif
