#pragma once

#include "component.h"
#include "Mat4x4.h"
#include <string>

struct ModelComponent : Component<ModelComponent>
{
	std::string filename;
	
	Vec3 aabbMin;
	Vec3 aabbMax;
	
	Mat4x4 objectToWorld = Mat4x4(true);
	
	virtual bool init(const std::vector<KeyValuePair> & params) override;
	
	void draw() const;
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
	}
};

#endif
