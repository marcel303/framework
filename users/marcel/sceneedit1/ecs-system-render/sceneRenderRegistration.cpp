#include "sceneRenderRegistration.h"

SceneRenderRegistrationBase * g_sceneRenderRegistrationList = nullptr;

SceneRenderRegistrationBase::SceneRenderRegistrationBase()
{
	next = g_sceneRenderRegistrationList;
	g_sceneRenderRegistrationList = this;
}
