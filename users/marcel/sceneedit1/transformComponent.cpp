#include "transformComponent.h"

bool TransformComponent::init()
{
	return true;
}

//

void RotateTransformComponent::tick(const float dt)
{
	// fetch transform component and update its rotation
	
	auto transformComponent = componentSet->find<TransformComponent>();
	
	if (transformComponent != nullptr)
		transformComponent->angleAxis.angle += speed * dt;
}
