#pragma once

#include "component.h"
#include "Vec3.h"

class Mat4x4;
class Surface;

namespace rOne
{
	struct ForwardLightingHelper;
	
	class ShadowMapDrawer;
}

struct LightComponent : Component<LightComponent>
{
	enum LightType
	{
		kLightType_Directional,
		kLightType_Point,
		kLightType_Spot,
		kLightType_AreaBox,
		kLightType_AreaSphere,
		kLightType_AreaRect,
		kLightType_AreaCircle
	};
	
	LightType type = kLightType_Point;
	float intensity = 1.f;             // linear light intensity
	Vec3 color = Vec3(1, 1, 1);        // light color encoded in the sRGB color space
	Vec3 bottomColor = Vec3(0, 0, 0);  // todo : rename to secondaryColor
	float farDistance = 10.f;          // for light attenuation for non-directional light types, and for directional and spot light shadow map rendering
	float attenuationBegin = 0.f;      // for light attenuation for non-directional light types
	float spotAngle = 90.f;            // the spot light angle in degrees
	bool castShadows = false;
	
	virtual void drawGizmo(ComponentDraw & draw) const override;
	virtual const char * getGizmoTexturePath() const override;
};

struct LightComponentMgr : ComponentMgr<LightComponent>
{
	rOne::ForwardLightingHelper * forwardLightingHelper = nullptr;
	
	rOne::ShadowMapDrawer * shadowMapDrawer = nullptr;
	
	bool enableShadowMaps = false;
	float shadowMapNearDistance = .05f; // the near distance (meters) to use for shadow map rendering
	
	virtual bool init() override;
	virtual void shut() override;
	
	void beforeDraw(const Mat4x4 & worldToView);
};

extern LightComponentMgr g_lightComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct LightComponentType : ComponentType<LightComponent>
{
	LightComponentType()
		: ComponentType("LightComponent", &g_lightComponentMgr)
	{
		tickPriority = kComponentPriority_Light;
		
		add("type", &LightComponent::type);
		add("intensity", &LightComponent::intensity)
			.limits(0.f, 100.f)
			.editingCurveExponential(4.f);
		add("color", &LightComponent::color)
			.addFlag<ComponentMemberFlag_EditorType_ColorSrgb>();
		add("bottomColor", &LightComponent::bottomColor)
			.addFlag<ComponentMemberFlag_EditorType_ColorSrgb>();
		add("farDistance", &LightComponent::farDistance);
		add("attenuationBegin", &LightComponent::attenuationBegin)
			.limits(0.f, 1.f);
		add("spotAngle", &LightComponent::spotAngle)
			.limits(0.f, 170.f);
		add("castShadows", &LightComponent::castShadows);
	}
	
	virtual void reflect(TypeDB & typeDB) override final
	{
		typeDB.addEnum<LightComponent::LightType>("LightComponent::LightType")
			.add("directional", LightComponent::kLightType_Directional)
			.add("point", LightComponent::kLightType_Point)
			.add("spot", LightComponent::kLightType_Spot)
			.add("areaBox", LightComponent::kLightType_AreaBox)
			.add("areaSphere", LightComponent::kLightType_AreaSphere)
			.add("areaRect", LightComponent::kLightType_AreaRect)
			.add("areaCircle", LightComponent::kLightType_AreaCircle);
	}
};

#endif
