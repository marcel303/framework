#include "transformComponent.h"

bool TransformComponent::init()
{
	return true;
}

//

void RotateTransformComponent::tick(const float dt)
{
	// fetch transform component and update its rotation
	
	auto transformComponent = componentSet->findComponent<TransformComponent>();
	
	if (transformComponent != nullptr)
		transformComponent->angleAxis.angle += speed * dt;
}
