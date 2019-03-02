#include "Mat4x4.h"
#include "Quat.h"
#include "scene.h"
#include "sceneNodeComponent.h"
#include "transformComponent.h"
#include <math.h>

void TransformComponentMgr::calculateTransformsTraverse(Scene & scene, SceneNode & node, const Mat4x4 & globalTransform) const
{
	auto * sceneNodeComp = node.components.find<SceneNodeComponent>();
	auto * transformComp = node.components.find<TransformComponent>();
	
	Mat4x4 newGlobalTransform;
	
	if (transformComp != nullptr)
	{
		Quat q;
		q.fromAxisAngle(transformComp->angleAxis.axis, transformComp->angleAxis.angle * float(M_PI) / 180.f);

		const Mat4x4 localTransform = Mat4x4(true).Translate(transformComp->position).Rotate(q).Scale(transformComp->scale);
		
		newGlobalTransform = globalTransform * localTransform;
	}
	else
	{
		newGlobalTransform = globalTransform;
	}
	
	if (sceneNodeComp != nullptr)
	{
		sceneNodeComp->objectToWorld = newGlobalTransform;
	}
	
	for (auto & childNodeId : node.childNodeIds)
	{
		auto childNodeItr = scene.nodes.find(childNodeId);
		
		Assert(childNodeItr != scene.nodes.end());
		if (childNodeItr != scene.nodes.end())
		{
			SceneNode & childNode = *childNodeItr->second;
			
			calculateTransformsTraverse(scene, childNode, newGlobalTransform);
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
		
		angle = fmodf(angle, 360.f);
		
		transformComponent->angleAxis.angle = angle;
	}
}
