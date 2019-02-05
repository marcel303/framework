#pragma once

#include "component.h"
#include "Mat4x4.h"
#include <string>

struct ModelComponent : Component<ModelComponent>
{
	std::string filename;
	float scale = 1.f;
	AngleAxis rotation;
	bool colorTexcoords = false;
	bool colorNormals = false;
	
	Vec3 modelAabbMin;
	Vec3 modelAabbMax;
	
	Vec3 aabbMin;
	Vec3 aabbMax;
	
	Mat4x4 _objectToWorld = Mat4x4(true); // todo : remove. use transform stored in SceneNode
	
	virtual bool init() override final;
	virtual void tick(const float dt) override final;
	
	void draw(const Mat4x4 & objectToWorld) const;
};

struct ModelComponentMgr : ComponentMgr<ModelComponent>
{
	void draw() const;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ModelComponentType : ComponentType<ModelComponent>
{
	ModelComponentType()
	{
		typeName = "ModelComponent";
		
		in("filename", &ModelComponent::filename);
		in("rotation", &ModelComponent::rotation);
		in("scale", &ModelComponent::scale)
			.setLimits(0.f, 100.f)
			.setEditingCurveExponential(2.f);
		in("colorTexcoords", &ModelComponent::colorTexcoords);
		in("colorNormals", &ModelComponent::colorNormals);
	}
};

#endif
