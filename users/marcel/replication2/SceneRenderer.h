#ifndef SCENERENDERER_H
#define SCENERENDERER_H
#pragma once

#include "Mesh.h"

class Scene;

class SceneRenderer
{
public:
	float m_camNear;
	float m_camFar;

	SceneRenderer(Scene* scene);
	~SceneRenderer();

	void Render();

	void RenderSolid();
	void RenderTransparent();
	void RenderAll();

	float GetAspect();
	void SetCameraPlanes(float nearDistance, float farDistance);

private:
	Scene* m_scene;
};

#endif
