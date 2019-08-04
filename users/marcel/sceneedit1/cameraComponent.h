#pragma once

#include "component.h"
#include "Mat4x4.h"
#include <string>

//

struct CameraComponent : Component<CameraComponent>
{
	float fov = 60.f;
	float nearDistance = .01f;
	float farDistance = 100.f;

	std::string controller;

	virtual void tick(const float dt) override final;
};

struct CameraComponentMgr : ComponentMgr<CameraComponent>
{
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct CameraComponentType : ComponentType<CameraComponent>
{
	CameraComponentType()
		: ComponentType("CameraComponent")
	{
		tickPriority = kComponentPriority_Camera;

		add("controller", &CameraComponent::controller);
		add("fov", &CameraComponent::fov);
		add("nearDistance", &CameraComponent::nearDistance);
		add("farDistance", &CameraComponent::farDistance);
	}
};

#endif
