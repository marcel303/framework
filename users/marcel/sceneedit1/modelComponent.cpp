#include "framework.h"
#include "modelComponent.h"

bool ModelComponent::init()
{
	Model(filename.c_str()).calculateAABB(aabbMin, aabbMax, true);
	
	return true;
}

void ModelComponent::draw(const Mat4x4 & objectToWorld) const
{
	if (filename.empty())
		return;
	
	gxPushMatrix();
	{
		gxMultMatrixf(objectToWorld.m_v);
		
		setColor(colorWhite);
		Model(filename.c_str()).draw();
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
