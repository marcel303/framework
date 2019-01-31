#include "framework.h"
#include "scene.h"
#include "transformComponent.h"
#include <math.h>

#include "modelComponent.h" // fixme : hack to set global transform on model component

bool TransformComponent::init()
{
	return true;
}

void TransformComponentMgr::calculateTransformsTraverse(Scene & scene, SceneNode & node) const
{
	gxPushMatrix();
	{
		auto transformComp = node.components.find<TransformComponent>();
		
		if (transformComp != nullptr)
		{
			gxTranslatef(
				transformComp->position[0],
				transformComp->position[1],
				transformComp->position[2]);
			
			gxRotatef(
				transformComp->angleAxis.angle * 180.f / float(M_PI),
				transformComp->angleAxis.axis[0],
				transformComp->angleAxis.axis[1],
				transformComp->angleAxis.axis[2]);
			
			gxScalef(
				transformComp->scale,
				transformComp->scale,
				transformComp->scale);
		}
		
		gxGetMatrixf(GX_MODELVIEW, node.objectToWorld.m_v);
		
		auto modelComp = node.components.find<ModelComponent>();
		
		if (modelComp != nullptr)
		{
			modelComp->_objectToWorld = node.objectToWorld;
		}
		
		for (auto & childNodeId : node.childNodeIds)
		{
			auto childNodeItr = scene.nodes.find(childNodeId);
			
			Assert(childNodeItr != scene.nodes.end());
			if (childNodeItr != scene.nodes.end())
			{
				SceneNode & childNode = *childNodeItr->second;
				
				calculateTransformsTraverse(scene, childNode);
			}
		}
	}
	gxPopMatrix();
}

void TransformComponentMgr::calculateTransforms(Scene & scene) const
{
	calculateTransformsTraverse(scene, scene.getRootNode());
}

//

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
