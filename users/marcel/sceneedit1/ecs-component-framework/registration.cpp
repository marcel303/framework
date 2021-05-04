#include "componentTypeDB.h"

#define DEFINE_COMPONENT_TYPES
#include "modelComponent.h"

ComponentTypeRegistration(ModelComponentType);

#include "sceneRenderRegistration.h"

struct SceneRenderRegistration_ModelComponent : SceneRenderRegistrationBase
{
	virtual void setup() override final
	{
		draw = [](const RenderPass pass)
		{
			switch (pass)
			{
			case kRenderPass_Opaque:
				g_modelComponentMgr.draw();
				break;
				
			case kRenderPass_Opaque_ForwardShaded:
				break;
				
			case kRenderPass_Translucent:
				break;
				
			case kRenderPass_Background:
			case kRenderPass_COUNT:
				break;
			}
		};
		
		drawShadow = [](const ShadowPass pass)
		{
			switch (pass)
			{
			case kShadowPass_Opaque:
				g_modelComponentMgr.draw();
				break;
				
			case kShadowPass_Translucent:
				break;
				
			case kShadowPass_COUNT:
				break;
			}
		};
	}
};

SceneRenderRegistration_ModelComponent g_SceneRenderRegistration_ModelComponent;

void linkFrameworkComponents() { }
