#pragma once

#include "gltf.h"
#include "gltf-draw.h" // for BoundingBox

#include "AngleAxis.h"
#include "component.h"
#include "Mat4x4.h"
#include <string>

struct GltfComponent : Component<GltfComponent>
{
	std::string filename;
	float scale = 1.f;
	AngleAxis rotation;
	bool centimetersToMeters = false;
	
	gltf::BoundingBox modelAabb;
	
	bool aabbIsValid = false;
	Vec3 aabbMin;
	Vec3 aabbMax;

	class GltfCacheElem * cacheElem = nullptr;
	
	virtual ~GltfComponent() override final;
	
	virtual bool init() override final;
	virtual void tick(const float dt) override final;
	
	virtual void propertyChanged(void * address) override final;
	
	void free();
	
	void drawOpaque(const Mat4x4 & objectToWorld) const;
	void drawTranslucent(const Mat4x4 & objectToWorld) const;
};

//

struct Scene;

struct GltfComponentMgr : ComponentMgr<GltfComponent>
{
	void drawOpaque() const;
	void drawTranslucent() const;
};

extern GltfComponentMgr g_gltfComponentMgr;


#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct GltfComponentType : ComponentType<GltfComponent>
{
	GltfComponentType()
		: ComponentType("GltfComponent", &g_gltfComponentMgr)
	{
		add("filename", &GltfComponent::filename)
			.addFlag(new ComponentMemberFlag_EditorType_FilePath);
		add("rotation", &GltfComponent::rotation);
		add("scale", &GltfComponent::scale)
			.limits(0.f, 100.f)
			.editingCurveExponential(2.f);
		add("centimetersToMeters", &GltfComponent::centimetersToMeters);
	}
};

#endif
