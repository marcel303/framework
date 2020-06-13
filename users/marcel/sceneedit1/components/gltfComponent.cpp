#include "framework.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "gltfComponent.h"
#include "sceneNodeComponent.h"

using namespace gltf;

GltfComponent::~GltfComponent()
{
	free();
}

bool GltfComponent::init()
{
	// load GLTF scene
	
	loadScene(filename.c_str(), scene);
	
	// initialize buffer cache
	
	bufferCache.init(scene);
	
	// compute AABB
	
	modelAabb = BoundingBox();
	calculateSceneMinMax(scene, modelAabb);
	
	// compute scaled AABB
	
	if (modelAabb.hasMinMax)
	{
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		aabbMin = modelAabb.min * finalScale;
		aabbMax = modelAabb.max * finalScale;
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
		
		// load GLTF scene
	
		loadScene(filename.c_str(), scene);
		
		// initialize buffer cache
		
		bufferCache.init(scene);
		
		// compute AABB
		
		modelAabb = BoundingBox();
		calculateSceneMinMax(scene, modelAabb);
	}
	
	if (address == &filename || address == &scale || address == &centimetersToMeters)
	{
		// compute scaled AABB
		
		if (modelAabb.hasMinMax)
		{
			const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
			
			aabbMin = modelAabb.min * finalScale;
			aabbMax = modelAabb.max * finalScale;
		}
	}
}

void GltfComponent::free()
{
	bufferCache.free();

	scene = gltf::Scene();
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
		
		drawScene(scene, &bufferCache, materialShaders, true);
		drawScene(scene, &bufferCache, materialShaders, false);
	}
	gxPopMatrix();
}

//

void GltfComponentMgr::draw() const
{
	for (auto * i = head; i != nullptr; i = i->next)
	{
		auto * sceneNodeComp = i->componentSet->find<SceneNodeComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
			i->draw(sceneNodeComp->objectToWorld);
	}
}
