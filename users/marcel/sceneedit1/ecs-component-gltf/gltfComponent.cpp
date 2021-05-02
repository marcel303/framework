#include "gltfCache.h"
#include "gltfComponent.h"

#include "lightComponent.h"
#include "sceneNodeComponent.h"

#include "gltf-draw.h"
#include "gltf-loader.h"

#include "forwardLighting.h"
#include "shadowMapDrawer.h"

#include "framework.h"

using namespace gltf;

GltfComponentMgr g_gltfComponentMgr;

GltfComponent::~GltfComponent()
{
	free();
}

bool GltfComponent::init()
{
	cacheElem = &gltfCache().findOrCreate(filename.c_str());

	// compute scaled AABB
	
	aabbIsValid = cacheElem->m_aabb->hasMinMax;
	
	if (cacheElem->m_aabb->hasMinMax)
	{
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		aabbMin = cacheElem->m_aabb->min * finalScale;
		aabbMax = cacheElem->m_aabb->max * finalScale;
	}
	
	return true;
}

void GltfComponent::tick(const float dt)
{
}

void GltfComponent::propertyChanged(void * address)
{
	if (address == &filename)
	{
		free();
		
		// update cache elem
		
		cacheElem = &gltfCache().findOrCreate(filename.c_str());
	}
	
	if (address == &filename || address == &scale || address == &centimetersToMeters)
	{
		// compute scaled AABB
		
		aabbIsValid = cacheElem->m_aabb->hasMinMax;
		
		if (cacheElem->m_aabb->hasMinMax)
		{
			const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
			
			aabbMin = cacheElem->m_aabb->min * finalScale;
			aabbMax = cacheElem->m_aabb->max * finalScale;
		}
	}
}

bool GltfComponent::getBoundingBox(Vec3 & min, Vec3 & max) const
{
	if (aabbIsValid)
	{
		min = aabbMin;
		max = aabbMax;
	}
	
	return aabbIsValid;
}

void GltfComponent::free()
{
	cacheElem = nullptr;
}

void GltfComponent::drawOpaque(const Mat4x4 & objectToWorld, const MaterialShaders & materialShaders) const
{
	if (filename.empty())
		return;
	
	gxPushMatrix();
	{
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		gxMultMatrixf(objectToWorld.m_v);
		gxRotatef(rotation.angle, rotation.axis[0], rotation.axis[1], rotation.axis[2]);
		gxScalef(finalScale, finalScale, finalScale);
		
		drawScene(*cacheElem->m_scene, cacheElem->m_bufferCache, materialShaders, true);
	}
	gxPopMatrix();
}

void GltfComponent::drawTranslucent(const Mat4x4 & objectToWorld, const MaterialShaders & materialShaders) const
{
	if (filename.empty())
		return;
	
	gxPushMatrix();
	{
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		gxMultMatrixf(objectToWorld.m_v);
		gxRotatef(rotation.angle, rotation.axis[0], rotation.axis[1], rotation.axis[2]);
		gxScalef(finalScale, finalScale, finalScale);
		
		drawScene(*cacheElem->m_scene, cacheElem->m_bufferCache, materialShaders, false);
	}
	gxPopMatrix();
}

//

static Shader s_metallicRoughnessShader;
static Shader s_specularGlossinessShader;

static void setMaterialShaders(MaterialShaders & materialShaders, const bool forwardShaded, const bool outputLinearColorSpace, const Mat4x4 & worldToView)
{
	if (forwardShaded)
	{
		s_metallicRoughnessShader = Shader("ecs-component/gltf/forward-pbr-metallicRoughness");
		s_specularGlossinessShader = Shader("ecs-component/gltf/forward-pbr-specularGlossiness");
	}
	else
	{
		s_metallicRoughnessShader = Shader("ecs-component/gltf/deferred-pbr-metallicRoughness");
		s_specularGlossinessShader = Shader("ecs-component/gltf/deferred-pbr-specularGlossiness");
	}
	
	materialShaders.metallicRoughnessShader = &s_metallicRoughnessShader;
	materialShaders.specularGlossinessShader = &s_specularGlossinessShader;
	
	Shader * shaders[2] =
		{
			&s_metallicRoughnessShader,
			&s_specularGlossinessShader
		};
	
	for (int i = 0; i < 2; ++i)
	{
		setShader(*shaders[i]);
		{
			int nextTextureUnit = 0;
			
			if (forwardShaded)
			{
				g_lightComponentMgr.forwardLightingHelper->setShaderData(*shaders[i], nextTextureUnit);
			}
			
			g_lightComponentMgr.shadowMapDrawer->setShaderData(*shaders[i], nextTextureUnit, worldToView);
			
			materialShaders.firstTextureUnit = nextTextureUnit;
			
			shaders[i]->setImmediate("outputLinearColorSpace", outputLinearColorSpace ? 1.f : 0.f);
		}
		clearShader();
		
		materialShaders.init();
	}
}

void GltfComponentMgr::drawOpaque() const
{
	if (_enableForwardShading)
		return;
		
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	
	MaterialShaders materialShaders;
	setMaterialShaders(materialShaders, _enableForwardShading, _outputLinearColorSpace, worldToView);
		
	for (auto * i = head; i != nullptr; i = i->next)
	{
		if (i->enabled == false)
			continue;
			
		auto * sceneNodeComp = i->componentSet->find<SceneNodeComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
			i->drawOpaque(sceneNodeComp->objectToWorld, materialShaders);
	}
}

void GltfComponentMgr::drawOpaque_ForwardShaded() const
{
	if (!_enableForwardShading)
		return;
		
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	
	MaterialShaders materialShaders;
	setMaterialShaders(materialShaders, _enableForwardShading, _outputLinearColorSpace, worldToView);
		
	for (auto * i = head; i != nullptr; i = i->next)
	{
		if (i->enabled == false)
			continue;
			
		auto * sceneNodeComp = i->componentSet->find<SceneNodeComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
			i->drawOpaque(sceneNodeComp->objectToWorld, materialShaders);
	}
}

void GltfComponentMgr::drawTranslucent() const
{
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	
	MaterialShaders materialShaders;
	setMaterialShaders(materialShaders, _enableForwardShading, false, worldToView);
	
	for (auto * i = head; i != nullptr; i = i->next)
	{
		if (i->enabled == false)
			continue;
			
		auto * sceneNodeComp = i->componentSet->find<SceneNodeComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
			i->drawTranslucent(sceneNodeComp->objectToWorld, materialShaders);
	}
}
