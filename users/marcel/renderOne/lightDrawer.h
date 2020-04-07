#pragma once

#include "Mat4x4.h"
#include "Vec3.h"
#include <vector>

namespace rOne
{
	struct LightDrawer
	{
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
		
		// methods to begin and end drawing deferred lights
		
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
		
		// methods for drawing deferred lights
		
		void drawDeferredAmbientLight(
			const Vec3 & lightColor,
			const float lightIntensity) const;
		
		void drawDeferredDirectionalLight(
			const Vec3 & lightDirection,
			const Vec3 & lightColorTop,
			const Vec3 & lightColorBottom,
			const float lightIntensity) const;
		
		void drawDeferredPointLight(
			const Vec3 & lightPosition,
			const float lightAttenuationBegin,
			const float lightAttenuationEnd,
			const Vec3 & lightColor,
			const float lightIntensity) const;

		void drawDeferredSpotLight(
			const Vec3 & lightPosition,
			const Vec3 & lightDirection,
			const float lightAngle,
			const float lightAttenuationBegin,
			const float lightAttenuationEnd,
			const Vec3 & lightColor,
			const float lightIntensity) const;
	};

	extern LightDrawer g_lightDrawer;
}
