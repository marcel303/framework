#pragma once

#include "component.h"

struct Scene;
struct SceneNode;

struct TransformComponent : Component<TransformComponent>
{
	Vec3 position;
	AngleAxis angleAxis;
	float scale = 1.f;
};

struct TransformComponentMgr : ComponentMgr<TransformComponent>
{
	void calculateTransformsTraverse(Scene & scene, SceneNode & node, const Mat4x4 & globalTransform) const;
	void calculateTransforms(Scene & scene) const;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct TransformComponentType : ComponentType<TransformComponent>
{
	TransformComponentType()
	{
		typeName = "TransformComponent";
		tickPriority = kComponentPriority_Transform;
		
		in("position", &TransformComponent::position);
		in("angleAxis", &TransformComponent::angleAxis);
		in("scale", &TransformComponent::scale)
			.setLimits(0.f, 10.f)
			.setEditingCurveExponential(2.f);
	}
};

#endif

// todo : move to its own source file

struct RotateTransformComponent : Component<RotateTransformComponent>
{
	float speed = 0.f;
	
	virtual void tick(const float dt) override final;
};

struct RotateTransformComponentMgr : ComponentMgr<RotateTransformComponent>
{
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct RotateTransformComponentType : ComponentType<RotateTransformComponent>
{
	RotateTransformComponentType()
	{
		typeName = "RotateTransformComponent";
		
		in("speed", &RotateTransformComponent::speed);
	}
};

#endif
