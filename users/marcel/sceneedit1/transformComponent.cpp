#include "Mat4x4.h"
#include "Quat.h"
#include "scene.h"
#include "transformComponent.h"

void TransformComponentMgr::calculateTransformsTraverse(Scene & scene, SceneNode & node, const Mat4x4 & globalTransform) const
{
	auto transformComp = node.components.find<TransformComponent>();
	
	if (transformComp != nullptr)
	{
		Quat q;
		q.fromAxisAngle(transformComp->angleAxis.axis, transformComp->angleAxis.angle);

		const Mat4x4 localTransform = Mat4x4(true).Translate(transformComp->position).Rotate(q).Scale(transformComp->scale);
		
		node.objectToWorld = globalTransform * localTransform;
	}
	else
	{
		node.objectToWorld = globalTransform;
	}
	
	for (auto & childNodeId : node.childNodeIds)
	{
		auto childNodeItr = scene.nodes.find(childNodeId);
		
		Assert(childNodeItr != scene.nodes.end());
		if (childNodeItr != scene.nodes.end())
		{
			SceneNode & childNode = *childNodeItr->second;
			
			calculateTransformsTraverse(scene, childNode, node.objectToWorld);
		}
	}
}

void TransformComponentMgr::calculateTransforms(Scene & scene) const
{
	calculateTransformsTraverse(scene, scene.getRootNode(), Mat4x4(true));
}

//

#include <math.h>

void RotateTransformComponent::tick(const float dt)
{
	// fetch transform component and update its rotation
	
	auto transformComponent = componentSet->find<TransformComponent>();
	
	if (transformComponent != nullptr)
	{
		float angle = transformComponent->angleAxis.angle;
		
		angle += speed * dt;
		
		angle = fmodf(angle, float(M_PI * 2.0));
		
		transformComponent->angleAxis.angle = angle;
	}
}
