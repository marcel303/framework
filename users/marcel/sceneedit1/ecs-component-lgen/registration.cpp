#include "componentTypeDB.h"

#include "sceneRenderRegistration.h"

#define DEFINE_COMPONENT_TYPES
#include "lgenComponent.h"

ComponentTypeRegistration(LgenComponentType);

struct SceneRenderRegistration_LgenComponent : SceneRenderRegistrationBase
{
	virtual void beginDraw(const SceneRenderParams & params) override final
	{
		//g_lgenComponentMgr._enableForwardShading = (params.hasDeferredLightingPass == false);
		//g_lgenComponentMgr._outputLinearColorSpace = params.outputToLinearColorSpace;
	}
	
	virtual void setup() override final
	{
		draw = [](const RenderPass pass)
		{
			switch (pass)
			{
			case kRenderPass_Opaque:
				g_lgenComponentMgr.drawOpaque();
				break;
				
			case kRenderPass_Opaque_ForwardShaded:
				g_lgenComponentMgr.drawOpaque_ForwardShaded();
				break;
			
			case kRenderPass_Translucent:
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
				g_lgenComponentMgr.drawOpaque();
				g_lgenComponentMgr.drawOpaque_ForwardShaded();
				break;
				
			case kShadowPass_Translucent:
				break;
				
			case kShadowPass_COUNT:
				break;
			}
		};
	}
};

SceneRenderRegistration_LgenComponent g_SceneRenderRegistration_LgenComponent;

void linkLgenComponents() { }
