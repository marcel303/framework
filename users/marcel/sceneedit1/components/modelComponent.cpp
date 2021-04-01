#include "framework.h"
#include "modelComponent.h"
#include "sceneNodeComponent.h"

ModelComponentMgr g_modelComponentMgr;

ModelComponent::~ModelComponent()
{
	delete model;
	model = nullptr;
}

bool ModelComponent::init()
{
	model = new Model(filename.c_str());
	
	hasModelAabb = model->calculateAABB(modelAabbMin, modelAabbMax, true);
	
	if (hasModelAabb)
	{
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		aabbMin = modelAabbMin * finalScale;
		aabbMax = modelAabbMax * finalScale;
	}
	
	return true;
}

void ModelComponent::tick(const float dt)
{
}

void ModelComponent::propertyChanged(void * address)
{
	if (address == &filename)
	{
		delete model;
		model = new Model(filename.c_str());
		
		hasModelAabb = model->calculateAABB(modelAabbMin, modelAabbMax, true);
	}
	
	if (address == &filename || address == &scale || address == &centimetersToMeters)
	{
		if (hasModelAabb)
		{
			const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
			
			aabbMin = modelAabbMin * finalScale;
			aabbMax = modelAabbMax * finalScale;
		}
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
		
		const float finalScale = scale * (centimetersToMeters ? .01f : 1.f);
		
		setColor(colorWhite);
		model->drawEx(Vec3(), rotation.axis, rotation.angle * float(M_PI) / 180.f, finalScale, drawFlags);
	}
	gxPopMatrix();
}

//

void ModelComponentMgr::draw() const
{
	for (auto * i = head; i != nullptr; i = i->next)
	{
		auto * sceneNodeComp = i->componentSet->find<SceneNodeComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
			i->draw(sceneNodeComp->objectToWorld);
	}
}
