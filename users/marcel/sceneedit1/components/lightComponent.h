#pragma once

#include "component.h"
#include "Vec3.h"

class Surface;

struct LightComponent : Component<LightComponent>
{
	enum LightType
	{
		kLightType_Point,
		kLightType_Directional,
		kLightType_Line
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
	Surface * lightAccumulationSurface = nullptr;
	
	virtual bool init() override;
	virtual void shut() override;
	
	void drawDeferredLights() const;
	void drawDeferredLightApplication(const int colorTextureId) const;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct LightComponentType : ComponentType<LightComponent>
{
	LightComponentType()
		: ComponentType("LightComponent")
	{
		tickPriority = kComponentPriority_Light;
		
		g_typeDB.addEnum<LightComponent::LightType>("LightComponent::LightType")
			.add("point", LightComponent::kLightType_Point)
			.add("directional", LightComponent::kLightType_Directional)
			.add("line", LightComponent::kLightType_Line);
		
		add("type", &LightComponent::type);
		in("intensity", &LightComponent::intensity)
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
