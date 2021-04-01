#include "cameraComponent.h"
#include "cameraResource.h"
#include "helpers2.h"

CameraComponentMgr g_cameraComponentMgr;

void CameraComponent::tick(const float dt)
{
	auto cameraController = findResource<CameraController>(controller.c_str());
	
	if (cameraController != nullptr)
	{
		float fovY = 0.f;
		float nearDistance = .01f;
		float farDistance = 100.f;
		
		cameraController->getProjectionProperties(
			fovY,
			nearDistance,
			farDistance);
	}
}
