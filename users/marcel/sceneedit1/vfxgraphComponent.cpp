#include "parameterComponent.h"
#include "vfxGraph.h"
#include "vfxGraphManager.h"
#include "vfxgraphComponent.h"

extern VfxgraphComponentMgr s_vfxgraphComponentMgr;

VfxgraphComponent::~VfxgraphComponent()
{
	s_vfxgraphComponentMgr.vfxGraphMgr->free(instance);
}

void VfxgraphComponent::tick(const float dt)
{
	if (instance != nullptr)
	{
		// todo : width and height. take inspiration from vfx graph's vfx graph node
		
		instance->vfxGraph->tick(640, 480, dt);
	}
}

bool VfxgraphComponent::init()
{
	if (path.empty())
	{
		// nothing to do
	}
	else
	{
		instance = s_vfxgraphComponentMgr.vfxGraphMgr->createInstance(path.c_str(), 640, 480);
		
		if (instance == nullptr)
		{
			return false;
		}
	}
	
	auto * paramComp = componentSet->find<ParameterComponent>();
	
	if (paramComp != nullptr)
	{
		paramComp->addBool("enabled", true);
		paramComp->addFloat("timeStepMultiplier", 1.f)
			->setLimits(0.f, 100.f)
			.setEditingCurveExponential(2.f);
	}
	
	return true;
}

void VfxgraphComponent::propertyChanged(void * address)
{
	if (address == &path)
	{
		s_vfxgraphComponentMgr.vfxGraphMgr->free(instance);
		
		instance = s_vfxgraphComponentMgr.vfxGraphMgr->createInstance(path.c_str(), 640, 480);
	}
}

//

VfxgraphComponentMgr::VfxgraphComponentMgr()
{
	vfxGraphMgr = new VfxGraphManager_Basic(true);
}

VfxgraphComponentMgr::~VfxgraphComponentMgr()
{
	delete vfxGraphMgr;
	vfxGraphMgr = nullptr;
}
