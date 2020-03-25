#pragma once

#include "Mat4x4.h"
#include "Vec3.h"
#include <vector>

enum LightType
{
	kLightType_Point,
	kLightType_Spot,
	kLightType_Directional
};

struct Light
{
	LightType type = kLightType_Point;
	Vec3 position;
	Vec3 direction;
	Vec3 color;
	Vec3 bottomColor; // for directional lights, this is the light when objects are lit from the 'bottom'
	float intensity = 1.f;
	float radius = 1.f;
	float falloffBegin = 1.f;
	float falloffCurve = 1.f;
};

struct LightMgr
{
	std::vector<const Light*> lights;

	void addLight(const Light * light);
	void removeLight(const Light * light);

	// deferred light functions
	
	struct DrawDeferredInfo
	{
		Mat4x4 worldToView;
		Mat4x4 viewToProjection;
		Mat4x4 projectionToView;
		int depthTextureId = 0;
		int normalTextureId = 0;
		int colorTextureId = 0;
		int specularColorTextureId = 0;
		int specularExponentTextureId = 0;
		int viewSx = 0;
		int viewSy = 0;
		bool enableStencilVolumes = false;
	};
	
	DrawDeferredInfo drawDeferredInfo;
	
	void drawDeferredBegin(
		const Mat4x4 & worldToView,
		const Mat4x4 & viewToProjection,
		const int depthTextureId,
		const int normalTextureId,
		const int colorTextureId,
		const int specularColorTextureId,
		const int specularExponentTextureId,
		const bool enableStencilVolumes);
	void drawDeferredEnd();
	
	void drawDeferredLights() const;
	
	void drawDeferredDirectionalLight(
		const Vec3 & lightDirection,
		const Vec3 & lightColorTop,
		const Vec3 & lightColorBottom) const;
	void drawDeferredPointLight(
		const Vec3 & lightPosition,
		const float lightAttenuationBegin,
		const float lightAttenuationEnd,
		const Vec3 & lightColor) const;
};

extern LightMgr g_lightMgr;
