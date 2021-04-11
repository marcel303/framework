#include "rotateTransformComponent.h"
#include "transformComponent.h"
#include <math.h>

RotateTransformComponentMgr g_rotateTransformComponentMgr;

void RotateTransformComponent::tick(const float dt)
{
	if (enabled == false)
		return;
		
	// fetch transform component and update its rotation
	
	auto * transformComponent = componentSet->find<TransformComponent>();
	
	if (transformComponent != nullptr)
	{
		float angle = transformComponent->angleAxis.angle;
		
		angle += speed * dt;
		
		angle = fmodf(angle, 360.f);
		
		transformComponent->angleAxis.angle = angle;
	}
}
