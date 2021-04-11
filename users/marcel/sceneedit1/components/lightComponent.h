#pragma once

#include "component.h"
#include "Vec3.h"

class Surface;

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
	float intensity = 1.f;
	Vec3 color = Vec3(1, 1, 1);
	Vec3 bottomColor = Vec3(0, 0, 0);
	float innerRadius = 1.f;
	float outerRadius = 10.f;
};

struct LightComponentMgr : ComponentMgr<LightComponent>
{
};

extern LightComponentMgr g_lightComponentMgr;


#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

extern TypeDB g_typeDB; // todo : remove

struct LightComponentType : ComponentType<LightComponent>
{
	LightComponentType()
		: ComponentType("LightComponent", &g_lightComponentMgr)
	{
		tickPriority = kComponentPriority_Light;
		
	// todo : need to resolve how to add enumeration types. and remove g_typeDB when resolved
	
		g_typeDB.addEnum<LightComponent::LightType>("LightComponent::LightType")
			.add("directional", LightComponent::kLightType_Directional)
			.add("point", LightComponent::kLightType_Point)
			.add("spot", LightComponent::kLightType_Spot)
			.add("areaBox", LightComponent::kLightType_AreaBox)
			.add("areaSphere", LightComponent::kLightType_AreaSphere)
			.add("areaRect", LightComponent::kLightType_AreaRect)
			.add("areaCircle", LightComponent::kLightType_AreaCircle);
		
		add("type", &LightComponent::type);
		add("intensity", &LightComponent::intensity)
			.limits(0.f, 100.f)
			.editingCurveExponential(4.f);
		add("color", &LightComponent::color)
			.addFlag<ComponentMemberFlag_EditorType_ColorSrgb>();
		add("bottomColor", &LightComponent::bottomColor)
			.addFlag<ComponentMemberFlag_EditorType_ColorSrgb>();
		add("innerRadius", &LightComponent::innerRadius);
		add("outerRadius", &LightComponent::outerRadius);
	}
};

#endif
