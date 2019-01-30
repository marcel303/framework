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
	aabbMin = modelAabbMin * scale;
	aabbMax = modelAabbMax * scale;
}

void ModelComponent::draw(const Mat4x4 & objectToWorld) const
{
	if (filename.empty())
		return;
	
	gxPushMatrix();
	{
		gxMultMatrixf(objectToWorld.m_v);
		
		setColor(colorWhite);
		Model(filename.c_str()).drawEx(Vec3(), rotation.axis, rotation.angle, scale);
	}
	gxPopMatrix();
}

//

void ModelComponentMgr::draw() const
{
	for (auto i = head; i != nullptr; i = i->next)
	{
		i->draw(i->_objectToWorld);
	}
}
