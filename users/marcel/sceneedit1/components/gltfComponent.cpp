#include "gltfCache.h"
#include "gltfComponent.h"

#include "sceneNodeComponent.h"

#include "gltf-draw.h"
#include "gltf-loader.h"

#include "framework.h"

using namespace gltf;

GltfComponentMgr g_gltfComponentMgr;

GltfComponent::~GltfComponent()
{
	free();
}

bool GltfComponent::init()
{
	cacheElem = &g_gltfCache.findOrCreate(filename.c_str());

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
		
		cacheElem = &g_gltfCache.findOrCreate(filename.c_str());
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

void GltfComponent::free()
{
	cacheElem = nullptr;
}

void GltfComponent::draw(const Mat4x4 & objectToWorld) const
{
	if (filename.empty())
		return;
	
	gxPushMatrix();
	{
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		gxMultMatrixf(objectToWorld.m_v);
		gxRotatef(rotation.angle, rotation.axis[0], rotation.axis[1], rotation.axis[2]);
		gxScalef(finalScale, finalScale, finalScale);
		
		MaterialShaders materialShaders;
		setDefaultMaterialShaders(materialShaders);
		
		drawScene(*cacheElem->m_scene, cacheElem->m_bufferCache, materialShaders, true);
		drawScene(*cacheElem->m_scene, cacheElem->m_bufferCache, materialShaders, false);
	}
	gxPopMatrix();
}

//

void GltfComponentMgr::draw() const
{
	for (auto * i = head; i != nullptr; i = i->next)
	{
		if (i->enabled == false)
			continue;
			
		auto * sceneNodeComp = i->componentSet->find<SceneNodeComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
			i->draw(sceneNodeComp->objectToWorld);
	}
}
