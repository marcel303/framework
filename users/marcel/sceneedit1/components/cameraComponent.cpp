#include "cameraComponent.h"
#include "cameraResource.h"
#include "helpers.h"

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
