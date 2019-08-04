#pragma once

#include "AngleAxis.h"
#include "component.h"

class Mat4x4;

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
		: ComponentType("TransformComponent")
	{
		tickPriority = kComponentPriority_Transform;
		
		add("position", &TransformComponent::position);
		add("angleAxis", &TransformComponent::angleAxis);
		in("scale", &TransformComponent::scale)
			.setLimits(0.f, 10.f)
			.setEditingCurveExponential(2.f);
	}
};

#endif
