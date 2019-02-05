#pragma once

#include "component.h"
#include "Mat4x4.h"

struct CameraComponent : Component<CameraComponent>
{
	float fov = 60.f;
	float nearDistance = .01f;
	float farDistance = 100.f;

	Mat4x4 projectionMatrix;
	Mat4x4 viewMatrix;

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
	{
		typeName = "CameraComponent";
		tickPriority = kComponentPriority_Camera;

		in("fov", &CameraComponent::fov);
		in("nearDistance", &CameraComponent::nearDistance);
		in("farDistance", &CameraComponent::farDistance);
	}
};

#endif
