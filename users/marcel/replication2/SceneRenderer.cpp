#include "Calc.h"
#include "Renderer.h"
#include "Scene.h"
#include "SceneRenderer.h"

SceneRenderer::SceneRenderer(Scene* scene)
{
	FASSERT(scene);

	m_scene = scene;

	SetCameraPlanes(1.0f, 1000.0f);
}

SceneRenderer::~SceneRenderer()
{
}

void SceneRenderer::Render()
{
	FASSERT(m_scene);

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	// Render color.
	gfx->Clear(BUFFER_ALL, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f);

	RenderAll();
}

void SceneRenderer::RenderSolid()
{
	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	gfx->RS(RS_DEPTHTEST, 1);
	gfx->RS(RS_DEPTHTEST_FUNC, CMP_LE);
	gfx->RS(RS_CULL, CULL_CW);

	m_scene->m_renderList.Render(RM_SOLID);

	gfx->RS(RS_CULL, CULL_NONE);
}

void SceneRenderer::RenderTransparent()
{
	// Render transparent objects.
	m_scene->m_renderList.Render(RM_BLENDED);
}

void SceneRenderer::RenderAll()
{
	RenderSolid();

	//RenderTransparent(); // fixme
}

float SceneRenderer::GetAspect()
{
	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	return gfx->GetRTH() / float(gfx->GetRTW());
}

void SceneRenderer::SetCameraPlanes(float nearDistance, float farDistance)
{
	m_camNear = nearDistance;
	m_camFar = farDistance;
}
