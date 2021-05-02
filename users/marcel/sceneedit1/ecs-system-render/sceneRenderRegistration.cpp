#include "sceneRenderRegistration.h"

SceneRenderRegistrationBase * g_sceneRenderRegistrationList = nullptr;

SceneRenderRegistrationBase::SceneRenderRegistrationBase()
{
	next = g_sceneRenderRegistrationList;
	g_sceneRenderRegistrationList = this;
}

// convenience functions

void sceneRender_setup()
{
	for (auto * renderRegistration = g_sceneRenderRegistrationList; renderRegistration != nullptr; renderRegistration = renderRegistration->next)
		renderRegistration->setup();
}

void sceneRender_beginDraw(const SceneRenderParams & params)
{
	for (auto * renderRegistration = g_sceneRenderRegistrationList; renderRegistration != nullptr; renderRegistration = renderRegistration->next)
		renderRegistration->beginDraw(params);
}

void sceneRender_endDraw()
{
	for (auto * renderRegistration = g_sceneRenderRegistrationList; renderRegistration != nullptr; renderRegistration = renderRegistration->next)
		renderRegistration->endDraw();
}

void sceneRender_draw(const RenderPass pass)
{
	for (auto * renderRegistration = g_sceneRenderRegistrationList; renderRegistration != nullptr; renderRegistration = renderRegistration->next)
		if (renderRegistration->draw != nullptr)
			renderRegistration->draw(pass);
}

void sceneRender_drawShadow(const ShadowPass pass)
{
	for (auto * renderRegistration = g_sceneRenderRegistrationList; renderRegistration != nullptr; renderRegistration = renderRegistration->next)
		if (renderRegistration->drawShadow != nullptr)
			renderRegistration->drawShadow(pass);
}
