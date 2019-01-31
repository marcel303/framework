#pragma once

#include "component.h"
#include "Mat4x4.h"

struct Scene;
struct SceneNode;

struct TransformComponent : Component<TransformComponent>
{
	Vec3 position;
	AngleAxis angleAxis;
	float scale = 1.f;
	
	virtual bool init() override;
};

struct TransformComponentMgr : ComponentMgr<TransformComponent>
{
	void calculateTransformsTraverse(Scene & scene, SceneNode & node) const;
	void calculateTransforms(Scene & scene) const;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct TransformComponentType : ComponentType<TransformComponent>
{
	TransformComponentType()
	{
		typeName = "TransformComponent";
		
		in("position", &TransformComponent::position);
		in("angleAxis", &TransformComponent::angleAxis);
		in("scale", &TransformComponent::scale)
			.setLimits(0.f, 10.f)
			.setEditingCurveExponential(2.f);
	}
};

#endif

// todo : move to its own source file

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct RotateTransformComponent : Component<RotateTransformComponent>
{
	float speed = 0.f;
	
	virtual void tick(const float dt)
	{
		// fetch transform component and update its rotation
		
		auto transformComponent = componentSet->findComponent<TransformComponent>();
		
		if (transformComponent != nullptr)
			transformComponent->angleAxis.angle += speed * dt;
	}
};

struct RotateTransformComponentMgr : ComponentMgr<RotateTransformComponent>
{
};

struct RotateTransformComponentType : ComponentType<RotateTransformComponent>
{
	RotateTransformComponentType()
	{
		typeName = "RotateTransformComponent";
		
		in("speed", &RotateTransformComponent::speed);
	}
};

#endif
