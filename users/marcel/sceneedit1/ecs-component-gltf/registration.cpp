#include "componentTypeDB.h"

#include "sceneRenderRegistration.h"

#define DEFINE_COMPONENT_TYPES
#include "gltfComponent.h"

ComponentTypeRegistration(GltfComponentType);

struct SceneRenderRegistration_GltfComponent : SceneRenderRegistrationBase
{
	/*
	// todo : let GltfComponent mgr perform a scene capture and draw meshes Z-sorted
	
	SceneToSceneCaptureSystem sceneToSceneCapture;
	sceneToSceneCapture.addCaptureFunctions(captureFunctions);
	
	RenderDrawableAllocator allocator(8 * 1024);
	
	SceneCapture capture(allocator);
	
	SceneCaptureToRenderOneSystem sceneCaptureToRender;
	sceneCaptureToRender.init();
	*/
	
	virtual void setup() override final
	{
		draw = [](const RenderPass pass)
		{
			switch (pass)
			{
			case kRenderPass_Opaque:
				g_gltfComponentMgr.drawOpaque();
				break;
				
			case kRenderPass_Opaque_ForwardShaded:
				g_gltfComponentMgr.drawOpaque_ForwardShaded();
				break;
				
			case kRenderPass_Translucent:
				g_gltfComponentMgr.drawTranslucent();
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
				g_gltfComponentMgr.drawOpaque();
				g_gltfComponentMgr.drawOpaque_ForwardShaded();
				break;
				
			case kShadowPass_Translucent:
				g_gltfComponentMgr.drawTranslucent();
				break;
				
			case kShadowPass_COUNT:
				break;
			}
		};
	}
};

SceneRenderRegistration_GltfComponent g_SceneRenderRegistration_GltfComponent;

void linkGltfComponents() { }
