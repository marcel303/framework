#include "lightComponent.h"

#include "componentDraw.h"

#include "sceneNodeComponent.h"

#include "Calc.h"

void LightComponent::drawGizmo(ComponentDraw & draw) const
{
	const Vec3 position = draw.sceneNodeComponent->objectToWorld.GetTranslation();
	
	const auto kOutlineColor = draw.makeColor(127, 127, 255);
	const auto kDirectionColor = draw.makeColor(255, 255, 127);
	
	switch (type)
	{
	case kLightType_Directional:
		{
			// draw direction lines
			
			const float kSpacing = .2f;
			const float kLength = 1.f;
			
			const Vec3 forward   = draw.sceneNodeComponent->objectToWorld.GetAxis(2).CalcNormalized() * kLength;
			const Vec3 tangent   = draw.sceneNodeComponent->objectToWorld.GetAxis(0).CalcNormalized() * kSpacing;
			const Vec3 bitangent = draw.sceneNodeComponent->objectToWorld.GetAxis(1).CalcNormalized() * kSpacing;
			
			const int count = 7;
			const float radius = 1.4f;
			
			draw.color(kDirectionColor);
			
			for (int i = 0; i < count; ++i)
			{
				const float angle = i / float(count) * Calc::m2PI;
				const float u = cosf(angle) * radius;
				const float v = sinf(angle) * radius;
				
				const Vec3 origin =
					position +
					tangent   * u +
					bitangent * v;
					
				const Vec3 target = origin + forward;
				
				draw.line(origin, target);
			}
		}
		break;
	case kLightType_Point:
		if (draw.isSelected)
		{
			draw.pushMatrix();
			{
				draw.translate(position);
				
				// draw sphere
				
				draw.color(kOutlineColor);

				const int resolution = 10;
				
				float x[resolution + 1];
				float z[resolution + 1];
				
				float y[resolution + 1];
				float r[resolution + 1];
				
				for (int u = 0; u <= resolution; ++u)
				{
					const float azimuth = u * Calc::m2PI / float(resolution);
					
					x[u] = cosf(azimuth);
					z[u] = sinf(azimuth);
				}
				
				const float radius = farDistance;
				const float radiusSq = radius * radius;
				
				for (int v = 0; v <= resolution; ++v)
				{
					const float elevation = v * Calc::mPI / float(resolution);
					
					y[v] = cosf(elevation) * radius;
					r[v] = sinf(elevation) * radius;
				}
				
				for (int u = 0; u < resolution; ++u)
				{
					for (int v = 0; v < resolution; ++v)
					{
						const float x1 = x[u];
						const float x2 = x[u + 1];
						const float z1 = z[u];
						const float z2 = z[u + 1];
						
						const float y1 = y[v];
						const float y2 = y[v + 1];
						
						const float r1 = r[v];
						const float r2 = r[v + 1];
						
						draw.line(
							x1 * r1, y1, z1 * r1,
							x1 * r2, y2, z1 * r2);
						
						draw.line(
							x1 * r1, y1, z1 * r1,
							x2 * r1, y1, z2 * r1);
					}
				}
			}
			draw.popMatrix();
		}
		break;
	case kLightType_Spot:
		if (draw.isSelected)
		{
			draw.pushMatrix();
			{
				draw.translate(position);
				
				// draw light cone outline
				
				const float alpha = tanf(spotAngle / 2.f / 180.f * float(M_PI));
				
				const Vec3 forward   = draw.sceneNodeComponent->objectToWorld.GetAxis(2).CalcNormalized();
				const Vec3 tangent   = draw.sceneNodeComponent->objectToWorld.GetAxis(0).CalcNormalized() * alpha;
				const Vec3 bitangent = draw.sceneNodeComponent->objectToWorld.GetAxis(1).CalcNormalized() * alpha;
				
				const int kNumSegments = 7;
				Vec3 * points = (Vec3*)alloca(kNumSegments * sizeof(Vec3));
				
				for (int i = 0; i < kNumSegments; ++i)
				{
					const float angle = i / float(kNumSegments) * Calc::m2PI;
					const float c = cosf(angle);
					const float s = sinf(angle);
					
					points[i] = (forward + tangent * c + bitangent * s) * farDistance;
				}
				
				const Vec3 origin;
				
				draw.color(kOutlineColor);
				
				for (int i = 0; i < kNumSegments; ++i)
				{
					const int index1 = i;
					const int index2 = i + 1 == kNumSegments ? 0 : i + 1;
					
					// cone side
					draw.line(
						origin,
						points[index1]);
					
					// cone base
					draw.line(
						points[index1],
						points[index2]);
				}
			}
			draw.popMatrix();
		}
		break;
	
	case kLightType_AreaBox:
		if (draw.isSelected)
		{
			draw.pushMatrix();
			{
				draw.multMatrix(draw.sceneNodeComponent->objectToWorld);
				
				draw.color(kOutlineColor);
				draw.lineCube(Vec3(), Vec3(1.f));
			}
			draw.popMatrix();
		}
		break;
		
	case kLightType_AreaSphere:
		if (draw.isSelected)
		{
			draw.pushMatrix();
			{
				draw.multMatrix(draw.sceneNodeComponent->objectToWorld);
				
				draw.color(kOutlineColor);
				draw.lineCircle(0, 0, 1.f);
				
				draw.rotate(90, Vec3(1, 0, 0));
				
				draw.color(kOutlineColor);
				draw.lineCircle(0, 0, 1.f);
				
				draw.rotate(90, Vec3(0, 1, 0));
				
				draw.color(kOutlineColor);
				draw.lineCircle(0, 0, 1.f);
			}
			draw.popMatrix();
		}
		break;
		
	case kLightType_AreaCircle:
		if (draw.isSelected)
		{
			draw.pushMatrix();
			{
				draw.multMatrix(draw.sceneNodeComponent->objectToWorld);
				
				draw.color(kOutlineColor);
				draw.lineCircle(0, 0, 1.f);
			}
			draw.popMatrix();
		}
		break;
		
	case kLightType_AreaRect:
		if (draw.isSelected)
		{
			draw.pushMatrix();
			{
				draw.multMatrix(draw.sceneNodeComponent->objectToWorld);
				
				draw.color(kOutlineColor);
				draw.lineRect(-1, -1, +1, +1);
			}
			draw.popMatrix();
		}
		break;
	}
}

const char * LightComponent::getGizmoTexturePath() const
{
	return "gizmo-light.png";
}

//

#include "lightComponent.h"

#include "componentDraw.h"

#include "sceneNodeComponent.h"

#include "sceneRenderRegistration.h"

#include "forwardLighting.h"
#include "shadowMapDrawer.h"

#include "Calc.h"
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
	// -- prepare shadow maps
	
	shadowMapDrawer->reset();
	
	if (enableShadowMaps)
	{
		int nextLightId = -1;
		
		for (auto * lightComp = head; lightComp != nullptr; lightComp = lightComp->next)
		{
			if (lightComp->enabled == false)
				continue;
			
			nextLightId++;
			
			if (lightComp->castShadows == false)
				continue;
			
			auto * sceneNodeComp = lightComp->componentSet->find<SceneNodeComponent>();
		
			const auto & objectToWorld = sceneNodeComp->objectToWorld;
			
			switch (lightComp->type)
			{
			case LightComponent::kLightType_Directional:
				{
					// todo : light comp needs shadow map near/far distances and extents
					shadowMapDrawer->addDirectionalLight(
						nextLightId,
						objectToWorld,
						lightComp->farDistance / 100.f,
						lightComp->farDistance,
						10.f);
				}
				break;
				
			case LightComponent::kLightType_Point:
				break;
			
			case LightComponent::kLightType_Spot:
				{
					// todo : light comp needs shadow map near/far distances
					shadowMapDrawer->addSpotLight(
						nextLightId,
						objectToWorld,
						lightComp->spotAngle / 180.f * float(M_PI),
						lightComp->farDistance / 100.f,
						lightComp->farDistance);
				}
				break;
				
			case LightComponent::kLightType_AreaBox:
			case LightComponent::kLightType_AreaSphere:
			case LightComponent::kLightType_AreaRect:
			case LightComponent::kLightType_AreaCircle:
				break;
			}
		}
		
		shadowMapDrawer->drawOpaque = []()
			{
				sceneRender_drawShadow(kShadowPass_Opaque);
			};
		
		shadowMapDrawer->drawTranslucent = []()
			{
				sceneRender_drawShadow(kShadowPass_Translucent);
			};
		
		shadowMapDrawer->enableColorShadows = false;
		
		shadowMapDrawer->drawShadowMaps(worldToView);
	}
	
	// -- perpare forward lighting
	
	forwardLightingHelper->reset();
	
	int nextLightId = -1;
	
	for (auto * lightComp = head; lightComp != nullptr; lightComp = lightComp->next)
	{
		if (lightComp->enabled == false)
			continue;
			
		nextLightId++;
		
		auto * sceneNodeComp = lightComp->componentSet->find<SceneNodeComponent>();
		
		const auto & objectToWorld = sceneNodeComp->objectToWorld;
		
		// note : the forward lighting helper will convert colors to linear color space for us
		
		switch (lightComp->type)
		{
		case LightComponent::kLightType_Directional:
			{
				forwardLightingHelper->addDirectionalLight(
					objectToWorld.GetAxis(2).CalcNormalized(),
					lightComp->color,
					lightComp->intensity,
					shadowMapDrawer->getShadowMapId(nextLightId));
			}
			break;
		
		case LightComponent::kLightType_Point:
			{
				forwardLightingHelper->addPointLight(
					objectToWorld.GetTranslation(),
					lightComp->farDistance * lightComp->attenuationBegin,
					lightComp->farDistance,
					lightComp->color,
					lightComp->intensity);
			}
			break;
		
		case LightComponent::kLightType_Spot:
			{
				forwardLightingHelper->addSpotLight(
					objectToWorld.GetTranslation(),
					objectToWorld.GetAxis(2).CalcNormalized(),
					lightComp->spotAngle / 180.f * float(M_PI),
					lightComp->farDistance,
					lightComp->color,
					lightComp->intensity,
					shadowMapDrawer->getShadowMapId(nextLightId));
			}
			break;
			
		case LightComponent::kLightType_AreaBox:
			{
				forwardLightingHelper->addAreaBoxLight(
					objectToWorld,
					lightComp->farDistance * lightComp->attenuationBegin,
					lightComp->farDistance,
					lightComp->color,
					lightComp->intensity);
			}
			break;
			
		case LightComponent::kLightType_AreaSphere:
			{
				forwardLightingHelper->addAreaSphereLight(
					objectToWorld,
					lightComp->farDistance * lightComp->attenuationBegin,
					lightComp->farDistance,
					lightComp->color,
					lightComp->intensity);
			}
			break;
			
		case LightComponent::kLightType_AreaRect:
			{
				forwardLightingHelper->addAreaRectLight(
					objectToWorld,
					lightComp->farDistance * lightComp->attenuationBegin,
					lightComp->farDistance,
					lightComp->color,
					lightComp->intensity);
			}
			break;
			
		case LightComponent::kLightType_AreaCircle:
			{
				forwardLightingHelper->addAreaCircleLight(
					objectToWorld,
					lightComp->farDistance * lightComp->attenuationBegin,
					lightComp->farDistance,
					lightComp->color,
					lightComp->intensity);
			}
			break;
		}
	}
	
	forwardLightingHelper->prepareShaderData(
		7,
		100.f,
		true,
		worldToView);
}
