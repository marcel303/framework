#pragma once

#include "AngleAxis.h"
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
	bool centimetersToMeters = false;
	
	Vec3 modelAabbMin;
	Vec3 modelAabbMax;
	
	Vec3 aabbMin;
	Vec3 aabbMax;
	
	virtual bool init() override final;
	virtual void tick(const float dt) override final;
	
	virtual void propertyChanged(void * address) override final;
	
	void draw(const Mat4x4 & objectToWorld) const;
};

//

struct Scene;

struct ModelComponentMgr : ComponentMgr<ModelComponent>
{
	void draw() const;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ModelComponentType : ComponentType<ModelComponent>
{
	ModelComponentType()
		: ComponentType("ModelComponent")
	{
		add("filename", &ModelComponent::filename);
		add("rotation", &ModelComponent::rotation);
		in("scale", &ModelComponent::scale)
			.setLimits(0.f, 100.f)
			.setEditingCurveExponential(2.f);
		add("colorTexcoords", &ModelComponent::colorTexcoords);
		add("colorNormals", &ModelComponent::colorNormals);
		add("centimetersToMeters", &ModelComponent::centimetersToMeters);
	}
};

#endif
