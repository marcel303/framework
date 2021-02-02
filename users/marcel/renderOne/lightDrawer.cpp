#include "framework.h"
#include "lightDrawer.h"
#include "lightVolumeBuilder.h" // computeSpotLightAabb
#include "srgbFunctions.h"

#define ENABLE_LIGHT_VOLUME_STENCIL true

namespace rOne
{
	LightDrawer g_lightDrawer;

	//

	void LightDrawer::drawDeferredBegin(
		const Mat4x4 & worldToView,
		const Mat4x4 & viewToProjection,
		const int depthTextureId,
		const int normalTextureId,
		const int colorTextureId,
		const int specularColorTextureId,
		const int specularExponentTextureId,
		const bool enableStencilVolumes)
	{
		drawDeferredInfo.worldToView = worldToView;
		drawDeferredInfo.viewToProjection = viewToProjection;
		drawDeferredInfo.projectionToView = viewToProjection.CalcInv();
		drawDeferredInfo.depthTextureId = depthTextureId;
		drawDeferredInfo.normalTextureId = normalTextureId;
		drawDeferredInfo.colorTextureId = colorTextureId;
		drawDeferredInfo.specularColorTextureId = specularColorTextureId;
		drawDeferredInfo.specularExponentTextureId = specularExponentTextureId;
		drawDeferredInfo.enableStencilVolumes = enableStencilVolumes;
	}

	void LightDrawer::drawDeferredEnd()
	{
		drawDeferredInfo = DrawDeferredInfo();
	}

	void LightDrawer::drawDeferredAmbientLight(
		const Vec3 & in_lightColor,
		const float lightIntensity) const
	{
		const Vec3 lightColor = srgbToLinear(in_lightColor) * lightIntensity;
		
		Shader shader("renderOne/deferred-lights/ambient");
		setShader(shader);
		shader.setTexture("depthTexture", 0, drawDeferredInfo.depthTextureId, false, true);
		shader.setTexture("normalTexture", 1, drawDeferredInfo.normalTextureId, false, true);
		shader.setTexture("colorTexture", 2, drawDeferredInfo.colorTextureId, false, true);
		shader.setTexture("specularColorTexture", 3, drawDeferredInfo.specularColorTextureId, false, true);
		shader.setTexture("specularExponentTexture", 4, drawDeferredInfo.specularExponentTextureId, false, true);
		shader.setImmediateMatrix4x4("projectionToView", drawDeferredInfo.projectionToView.m_v);
		shader.setImmediate("lightColor", lightColor[0], lightColor[1], lightColor[2]);
		drawRect(-1, -1, +1, +1);
		clearShader();
	}

	void LightDrawer::drawDeferredDirectionalLight(
		const Vec3 & lightDirection,
		const Vec3 & in_lightColor1,
		const Vec3 & in_lightColor2,
		const float lightIntensity) const
	{
		AssertMsg(fabsf(lightDirection.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized");
		
		const Vec3 lightDir_view = -drawDeferredInfo.worldToView.Mul3(lightDirection).CalcNormalized();
		const Vec3 lightColor1 = srgbToLinear(in_lightColor1) * lightIntensity;
		const Vec3 lightColor2 = srgbToLinear(in_lightColor2) * lightIntensity;

		Shader shader("renderOne/deferred-lights/directional");
		setShader(shader);
		shader.setTexture("depthTexture", 0, drawDeferredInfo.depthTextureId, false, true);
		shader.setTexture("normalTexture", 1, drawDeferredInfo.normalTextureId, false, true);
		shader.setTexture("colorTexture", 2, drawDeferredInfo.colorTextureId, false, true);
		shader.setTexture("specularColorTexture", 3, drawDeferredInfo.specularColorTextureId, false, true);
		shader.setTexture("specularExponentTexture", 4, drawDeferredInfo.specularExponentTextureId, false, true);
		shader.setImmediateMatrix4x4("projectionToView", drawDeferredInfo.projectionToView.m_v);
		shader.setImmediate("lightDir_view", lightDir_view[0], lightDir_view[1], lightDir_view[2]);
		shader.setImmediate("lightColor1", lightColor1[0], lightColor1[1], lightColor1[2]);
		shader.setImmediate("lightColor2", lightColor2[0], lightColor2[1], lightColor2[2]);
		drawRect(-1, -1, +1, +1);
		clearShader();
	}

	#if ENABLE_LIGHT_VOLUME_STENCIL
	static void stencilVolumeDrawBegin(const LightDrawer::DrawDeferredInfo & drawDeferredInfo)
	{
		const Mat4x4 & viewToProjection = drawDeferredInfo.viewToProjection;
		const Mat4x4 & worldToView = drawDeferredInfo.worldToView;
		
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		gxLoadMatrixf(viewToProjection.m_v);
		gxScalef(1, -1, 1);
		
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
		gxLoadMatrixf(worldToView.m_v);
		
		updateCullFlip();
		
		pushDepthTest(true, DEPTH_LESS, false);
		
		clearStencil(0x01, 0xff); // todo : clear stencil once at the start of deferred draw, and clear during light draw
		setStencilTest()
			.comparison(GX_STENCIL_FUNC_ALWAYS, 0, 0)
			.op(GX_STENCIL_FACE_FRONT, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_INC, GX_STENCIL_OP_KEEP)
			.op(GX_STENCIL_FACE_BACK,  GX_STENCIL_OP_KEEP, GX_STENCIL_OP_DEC, GX_STENCIL_OP_KEEP);
		
		pushColorWriteMask(0, 0, 0, 0);
	}

	static void stencilVolumeDrawEnd()
	{
		popColorWriteMask();
		clearStencilTest();
		popDepthTest();
		
		gxMatrixMode(GX_PROJECTION);
		gxPopMatrix();
		gxMatrixMode(GX_MODELVIEW);
		gxPopMatrix();
	}

	static void stencilVolumeTestBegin()
	{
		setStencilTest()
			.comparison(GX_STENCIL_FUNC_EQUAL, 0, 0xff)
			.op(GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP);
	}

	static void stencilVolumeTestEnd()
	{
		clearStencilTest();
	}
	#endif

	void LightDrawer::drawDeferredPointLight(
		const Vec3 & lightPosition,
		const float lightAttenuationBegin,
		const float lightAttenuationEnd,
		const Vec3 & in_lightColor,
		const float lightIntensity) const
	{
	#if ENABLE_LIGHT_VOLUME_STENCIL
		if (drawDeferredInfo.enableStencilVolumes)
		{
			stencilVolumeDrawBegin(drawDeferredInfo);
			{
				setColor(colorRed);
				fillCube(lightPosition, Vec3(lightAttenuationEnd, lightAttenuationEnd, lightAttenuationEnd));
			}
			stencilVolumeDrawEnd();

			stencilVolumeTestBegin();
		
			//setColor(0, 127, 0);
			//drawRect(-1, -1, +1, +1);
		}
	#endif
		
		const Vec3 lightPos_view = drawDeferredInfo.worldToView.Mul4(lightPosition);
		const Vec3 lightColor = srgbToLinear(in_lightColor) * lightIntensity;

		Shader shader("renderOne/deferred-lights/point");
		setShader(shader);
		shader.setTexture("depthTexture", 0, drawDeferredInfo.depthTextureId, false, true);
		shader.setTexture("normalTexture", 1, drawDeferredInfo.normalTextureId, false, true);
		shader.setTexture("colorTexture", 2, drawDeferredInfo.colorTextureId, false, true);
		shader.setTexture("specularColorTexture", 3, drawDeferredInfo.specularColorTextureId, false, true);
		shader.setTexture("specularExponentTexture", 4, drawDeferredInfo.specularExponentTextureId, false, true);
		shader.setImmediateMatrix4x4("projectionToView", drawDeferredInfo.projectionToView.m_v);
		shader.setImmediate("lightPosition_view", lightPos_view[0], lightPos_view[1], lightPos_view[2]);
		shader.setImmediate("lightColor", lightColor[0], lightColor[1], lightColor[2]);
		shader.setImmediate("lightAttenuationParams", lightAttenuationBegin, lightAttenuationEnd);
		{
			drawRect(-1, -1, +1, +1);
		}
		clearShader();
		
	#if ENABLE_LIGHT_VOLUME_STENCIL
		if (drawDeferredInfo.enableStencilVolumes)
		{
			stencilVolumeTestEnd();
		}
	#endif
	}

	void LightDrawer::drawDeferredSpotLight(
		const Vec3 & lightPosition,
		const Vec3 & lightDirection,
		const float lightAngle,
		const float lightAttenuationBegin,
		const float lightAttenuationEnd,
		const Vec3 & in_lightColor,
		const float lightIntensity) const
	{
		AssertMsg(fabsf(lightDirection.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized");
		
	#if ENABLE_LIGHT_VOLUME_STENCIL
		if (drawDeferredInfo.enableStencilVolumes)
		{
			stencilVolumeDrawBegin(drawDeferredInfo);
			{
				Vec3 min;
				Vec3 max;
				LightVolumeBuilder::computeSpotLightAabb(lightPosition, lightDirection, lightAngle, lightAttenuationEnd, min, max);
				
				setColor(colorRed);
				fillCube((min + max) / 2.f, (max - min) / 2.f);
				//fillCube(lightPosition, Vec3(lightAttenuationEnd, lightAttenuationEnd, lightAttenuationEnd));
			}
			stencilVolumeDrawEnd();

			stencilVolumeTestBegin();
		
			//setColor(0, 127, 0);
			//drawRect(-1, -1, +1, +1);
		}
	#endif
		
		const Vec3 lightPos_view = drawDeferredInfo.worldToView.Mul4(lightPosition);
		const Vec3 lightDir_view = drawDeferredInfo.worldToView.Mul3(lightDirection);
		const Vec3 lightColor = srgbToLinear(in_lightColor) * lightIntensity;

		Shader shader("renderOne/deferred-lights/spot");
		setShader(shader);
		shader.setTexture("depthTexture", 0, drawDeferredInfo.depthTextureId, false, true);
		shader.setTexture("normalTexture", 1, drawDeferredInfo.normalTextureId, false, true);
		shader.setTexture("colorTexture", 2, drawDeferredInfo.colorTextureId, false, true);
		shader.setTexture("specularColorTexture", 3, drawDeferredInfo.specularColorTextureId, false, true);
		shader.setTexture("specularExponentTexture", 4, drawDeferredInfo.specularExponentTextureId, false, true);
		shader.setImmediateMatrix4x4("projectionToView", drawDeferredInfo.projectionToView.m_v);
		shader.setImmediate("lightPosition_view", lightPos_view[0], lightPos_view[1], lightPos_view[2]);
		shader.setImmediate("lightDirection_view", lightDir_view[0], lightDir_view[1], lightDir_view[2]);
		shader.setImmediate("lightColor", lightColor[0], lightColor[1], lightColor[2]);
		shader.setImmediate("lightAttenuationParams", lightAttenuationBegin, lightAttenuationEnd, cosf(lightAngle / 2.f));
		{
			drawRect(-1, -1, +1, +1);
		}
		clearShader();
		
	#if ENABLE_LIGHT_VOLUME_STENCIL
		if (drawDeferredInfo.enableStencilVolumes)
		{
			stencilVolumeTestEnd();
		}
	#endif
	}
}
