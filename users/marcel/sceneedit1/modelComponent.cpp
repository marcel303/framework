#include "framework.h"
#include "modelComponent.h"

bool ModelComponent::init()
{
	Model(filename.c_str()).calculateAABB(modelAabbMin, modelAabbMax, true);
	
	aabbMin = modelAabbMin * scale;
	aabbMax = modelAabbMax * scale;
	
	return true;
}

void ModelComponent::tick(const float dt)
{
}

void ModelComponent::propertyChanged(void * address)
{
	if (address == &filename)
	{
		Model(filename.c_str()).calculateAABB(modelAabbMin, modelAabbMax, true);
		
		aabbMin = modelAabbMin * scale;
		aabbMax = modelAabbMax * scale;
	}
	else if (address == &scale)
	{
		aabbMin = modelAabbMin * scale;
		aabbMax = modelAabbMax * scale;
	}
}

void ModelComponent::draw(const Mat4x4 & objectToWorld) const
{
	if (filename.empty())
		return;
	
	gxPushMatrix();
	{
		gxMultMatrixf(objectToWorld.m_v);
		
		const int drawFlags =
			DrawMesh |
			(DrawColorTexCoords * colorTexcoords) |
			(DrawColorNormals * colorNormals);
			
		setColor(colorWhite);
		Model(filename.c_str()).drawEx(Vec3(), rotation.axis, rotation.angle, scale, drawFlags);
	}
	gxPopMatrix();
}

//

#include "scene.h"
#include "transformComponent.h"

void ModelComponentMgr::draw(const Scene & scene) const
{
#if 1
	for (auto & node_itr : scene.nodes)
	{
		auto & node = node_itr.second;
		
		auto * modelComp = node->components.find<ModelComponent>();
		
		if (modelComp != nullptr)
			modelComp->draw(node->objectToWorld);
	}
#else
	for (auto * i = head; i != nullptr; i = i->next)
	{
		i->draw(i->_objectToWorld);
	}
#endif
}
