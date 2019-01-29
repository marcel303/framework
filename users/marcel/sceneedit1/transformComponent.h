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
	
	virtual bool init(const std::vector<KeyValuePair> & params) override;
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
