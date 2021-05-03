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
	
	virtual const char * getGizmoTexturePath() const override
	{
		return "gizmo-camera.png";
	}
};

struct CameraComponentMgr : ComponentMgr<CameraComponent>
{
};

extern CameraComponentMgr g_cameraComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct CameraComponentType : ComponentType<CameraComponent>
{
	CameraComponentType()
		: ComponentType("CameraComponent", &g_cameraComponentMgr)
	{
		tickPriority = kComponentPriority_Camera;

		add("fov", &CameraComponent::fov);
		add("nearDistance", &CameraComponent::nearDistance);
		add("farDistance", &CameraComponent::farDistance);
	}
};

#endif
