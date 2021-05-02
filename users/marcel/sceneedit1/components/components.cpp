#include "componentTypeDB.h"

#define DEFINE_COMPONENT_TYPES
#include "cameraComponent.h"
#include "lightComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "rotateTransformComponent.h"
#include "transformComponent.h"

ComponentTypeRegistration(CameraComponentType);
ComponentTypeRegistration(LightComponentType);
ComponentTypeRegistration(ModelComponentType);
ComponentTypeRegistration(ParameterComponentType);
ComponentTypeRegistration(RotateTransformComponentType);
ComponentTypeRegistration(TransformComponentType);

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

void linkEcsComponents() { }
