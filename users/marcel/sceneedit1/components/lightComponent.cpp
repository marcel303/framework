#include "lightComponent.h"

#include "sceneNodeComponent.h"

#include "gltfComponent.h"
#include "modelComponent.h"

#include "forwardLighting.h"
#include "shadowMapDrawer.h"

#include "Mat4x4.h"

LightComponentMgr g_lightComponentMgr;

bool LightComponentMgr::init()
{
	forwardLightingHelper = new rOne::ForwardLightingHelper();
	
	shadowMapDrawer = new rOne::ShadowMapDrawer();
	shadowMapDrawer->alloc(4, 512); // todo : avoid shadow map drawer overhead when shadow maps are disabled
	
	return true;
}

void LightComponentMgr::shut()
{
	shadowMapDrawer->free();
	delete shadowMapDrawer;
	shadowMapDrawer = nullptr;
	
	delete forwardLightingHelper;
	forwardLightingHelper = nullptr;
}

void LightComponentMgr::beforeDraw(const Mat4x4 & worldToView)
{
	forwardLightingHelper->reset();
	
	shadowMapDrawer->reset();
	
	int nextShadowMapId = 0;
	
	for (auto * lightComp = head; lightComp != nullptr; lightComp = lightComp->next)
	{
		if (lightComp->enabled == false)
			continue;
			
		auto * sceneNodeComp = lightComp->componentSet->find<SceneNodeComponent>();
		
		const auto & objectToWorld = sceneNodeComp->objectToWorld;
		
		// note : the forward lighting helper will convert colors to linear color space for us
	
		switch (lightComp->type)
		{
		case LightComponent::kLightType_Directional:
			forwardLightingHelper->addDirectionalLight(
				objectToWorld.GetAxis(2).CalcNormalized(),
				lightComp->color,
				lightComp->intensity,
				enableShadowMaps && lightComp->castShadows ? nextShadowMapId : -1);
			if (enableShadowMaps && lightComp->castShadows)
			{
				// todo : light comp needs shadow map near/far distances and extents
				shadowMapDrawer->addDirectionalLight(
					nextShadowMapId,
					objectToWorld,
					lightComp->farDistance / 100.f,
					lightComp->farDistance,
					10.f);
				nextShadowMapId++;
			}
			break;
		
		case LightComponent::kLightType_Point:
			forwardLightingHelper->addPointLight(
				objectToWorld.GetTranslation(),
				lightComp->farDistance * lightComp->attenuationBegin,
				lightComp->farDistance,
				lightComp->color,
				lightComp->intensity);
			break;
		
		case LightComponent::kLightType_Spot:
			forwardLightingHelper->addSpotLight(
				objectToWorld.GetTranslation(),
				objectToWorld.GetAxis(2).CalcNormalized(),
				lightComp->spotAngle / 180.f * float(M_PI),
				lightComp->farDistance,
				lightComp->color,
				lightComp->intensity,
				enableShadowMaps && lightComp->castShadows ? nextShadowMapId : -1);
			if (enableShadowMaps && lightComp->castShadows)
			{
				// todo : light comp needs shadow map near/far distances
				shadowMapDrawer->addSpotLight(
					nextShadowMapId,
					objectToWorld,
					lightComp->spotAngle / 180.f * float(M_PI),
					lightComp->farDistance / 100.f,
					lightComp->farDistance);
				nextShadowMapId++;
			}
			break;
			
		case LightComponent::kLightType_AreaBox:
			forwardLightingHelper->addAreaBoxLight(
				objectToWorld,
				lightComp->farDistance * lightComp->attenuationBegin,
				lightComp->farDistance,
				lightComp->color,
				lightComp->intensity);
			break;
			
		case LightComponent::kLightType_AreaSphere:
			forwardLightingHelper->addAreaSphereLight(
				objectToWorld,
				lightComp->farDistance * lightComp->attenuationBegin,
				lightComp->farDistance,
				lightComp->color,
				lightComp->intensity);
			break;
			
		case LightComponent::kLightType_AreaRect:
			forwardLightingHelper->addAreaRectLight(
				objectToWorld,
				lightComp->farDistance * lightComp->attenuationBegin,
				lightComp->farDistance,
				lightComp->color,
				lightComp->intensity);
			break;
			
		case LightComponent::kLightType_AreaCircle:
			forwardLightingHelper->addAreaCircleLight(
				objectToWorld,
				lightComp->farDistance * lightComp->attenuationBegin,
				lightComp->farDistance,
				lightComp->color,
				lightComp->intensity);
			break;
		}
	}
	
	forwardLightingHelper->prepareShaderData(7, 20.f, true, worldToView);
	
	if (enableShadowMaps)
	{
	// todo : component mgrs should register for opaque, forward and translucent draw somewhere ? currently the calls to draw stuff are all over the place
	
		shadowMapDrawer->drawOpaque = []()
			{
			// todo : component mgrs should know this is a shadow map pass, so they can optimize drawing, and not draw geometry not tagged as shadow casting
			
				g_gltfComponentMgr.drawOpaque();
				
				g_gltfComponentMgr.drawOpaque_ForwardShaded();
				
				g_modelComponentMgr.draw();
			};
		
		shadowMapDrawer->drawTranslucent = []()
			{
				g_gltfComponentMgr.drawTranslucent();
			};
		
		shadowMapDrawer->enableColorShadows = false;
		
		shadowMapDrawer->drawShadowMaps(worldToView);
	}
}
