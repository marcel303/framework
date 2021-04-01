#pragma once

#include "AngleAxis.h"
#include "component.h"
#include "Mat4x4.h"
#include <string>

class Model;

struct ModelComponent : Component<ModelComponent>
{
	std::string filename;
	float scale = 1.f;
	AngleAxis rotation;
	bool colorTexcoords = false;
	bool colorNormals = false;
	bool centimetersToMeters = false;
	
	bool hasModelAabb = false;
	Vec3 modelAabbMin;
	Vec3 modelAabbMax;
	
	Vec3 aabbMin;
	Vec3 aabbMax;
	
	Model * model = nullptr;
	
	virtual ~ModelComponent() override;
	
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

extern ModelComponentMgr g_modelComponentMgr;


#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct ModelComponentType : ComponentType<ModelComponent>
{
	ModelComponentType()
		: ComponentType("ModelComponent", &g_modelComponentMgr)
	{
		add("filename", &ModelComponent::filename)
			.addFlag(new ComponentMemberFlag_EditorType_FilePath);
		add("rotation", &ModelComponent::rotation);
		add("scale", &ModelComponent::scale)
			.limits(0.f, 100.f)
			.editingCurveExponential(2.f);
		add("colorTexcoords", &ModelComponent::colorTexcoords);
		add("colorNormals", &ModelComponent::colorNormals);
		add("centimetersToMeters", &ModelComponent::centimetersToMeters);
	}
};

#endif
