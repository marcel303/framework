#ifndef SCENERENDERER_H
#define SCENERENDERER_H
#pragma once

#include "Mesh.h"

#define TEST_VOLUME_SPHERES 0

class Scene;

class SceneRenderer
{
public:
	SceneRenderer(Scene* scene);
	~SceneRenderer();

	void Render();

	/* TODO: Private --> */
	void RenderLightDepth(class EntityLight* light);
	void RenderLightLight(class EntityLight* light);
	void RenderLightPass(class EntityLight* light);
	void RenderSolid();
	void RenderLight();
	void RenderTransparent();
	void RenderAll();
	void GetSpotLightMatrices(class EntityLight* light, Mat4x4& out_matP, Mat4x4& out_matV);
	void GetPSMLightMatrices(class EntityLight* light, Mat4x4& out_matP, Mat4x4& out_matV);
	void GetSSMCubeMatrix(class EntityLight* light, Mat4x4& out_matP);
	float GetAspect();
	void SetCameraPlanes(float nearDistance, float farDistance);
	void SetShadowMapSize(int size);
	void BlurTexRectR(ResTexRectR* src, ResTexRectR* dst);
	float m_camNear;
	float m_camFar;
	ShTexRectR m_lightTexA; // Light accum buffer.
	ShTexD m_lightTexD; // Light depth buffer.
	ShTexRectR m_blurTex;
	ShShader m_plainShader;
	ShShader m_blurShader;
	/* <-- TODO: Private */

private:
	Scene* m_scene;

	// SSM cube map shadow method.
	ShShader m_ssmCubeDepthShader;
	ShShader m_ssmCubeLightShader;
	ShTexCR m_ssmCubeTex;

	// PSM shadow method.
	ShShader m_psmDepthShader;
	ShShader m_psmLightShader;
	ShTexR m_psmTex;

	// SSM shadow method.
	ShShader m_ssmDepthShader;
	ShShader m_ssmLightShader;
	ShTexR m_ssmTex;

#if TEST_VOLUME_SPHERES
	// Volume spheres.
	ShShader m_vol_sphere;
	ShShader m_vol_depthShader;
	Mesh m_vol_mesh;
	ShTexR m_vol_depth;
#endif
};

#endif
