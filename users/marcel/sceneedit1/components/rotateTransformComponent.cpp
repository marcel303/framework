#include "rotateTransformComponent.h"
#include "transformComponent.h"
#include <math.h>

void RotateTransformComponent::tick(const float dt)
{
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
