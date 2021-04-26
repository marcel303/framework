#include "parameterComponent.h"
#include "helpers.h" // g_resourceDatabase

// vfxgraph
#include "vfxGraph.h"
#include "vfxGraphManager.h"
#include "vfxgraphComponent.h"

// framework
#include "framework.h"

VfxgraphComponentMgr g_vfxgraphComponentMgr;

VfxgraphComponent::~VfxgraphComponent()
{
	g_vfxgraphComponentMgr.vfxGraphMgr->free(instance);
	
	g_resourceDatabase.remove(&textureResource);
}

void VfxgraphComponent::tick(const float dt)
{
	if (instance != nullptr)
	{
		// todo : width and height. take inspiration from vfx graph's vfx graph node
		
		instance->vfxGraph->tick(surfaceWidth, surfaceHeight, dt);
		
		textureResource.texture = instance->vfxGraph->traverseDraw();
	}
}

bool VfxgraphComponent::init()
{
// fixme : this assumes scene node ids are global; which they are not. resource DB should be a member of scene ?
	//g_resourceDatabase.addComponentResource(componentSet->id, "texture", &textureResource);
	
	if (path.empty())
	{
		// nothing to do
	}
	else
	{
		instance = g_vfxgraphComponentMgr.vfxGraphMgr->createInstance(
			path.c_str(),
			surfaceWidth,
			surfaceHeight);
		
		if (instance == nullptr)
		{
			return false;
		}
	}
	
	if (surfaceWidth > 0 && surfaceHeight > 0)
	{
		surface = new Surface(surfaceWidth, surfaceHeight, false);
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
		g_vfxgraphComponentMgr.vfxGraphMgr->free(instance);
		
		instance = g_vfxgraphComponentMgr.vfxGraphMgr->createInstance(path.c_str(), 640, 480);
	}
	else if (address == &surfaceWidth || address == &surfaceHeight)
	{
		delete surface;
		surface = nullptr;
		
		if (surfaceWidth > 0 && surfaceHeight > 0)
		{
			surface = new Surface(surfaceWidth, surfaceHeight, false);
		}
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
