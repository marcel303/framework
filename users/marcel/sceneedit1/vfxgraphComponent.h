#pragma once

#include "component.h"
#include <string>

struct VfxGraphInstance;
struct VfxGraphManager_Basic;

struct VfxgraphComponent : Component<VfxgraphComponent>
{
	std::string path;
	
	VfxGraphInstance * instance = nullptr;
	
	virtual ~VfxgraphComponent() override final;
	
	virtual void tick(const float dt) override final;
	virtual bool init() override final;
	
	virtual void propertyChanged(void * address) override final;
};

struct VfxgraphComponentMgr : ComponentMgr<VfxgraphComponent>
{
	VfxGraphManager_Basic * vfxGraphMgr = nullptr;
	
	VfxgraphComponentMgr();
	virtual ~VfxgraphComponentMgr() override final;
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct VfxgraphComponentType : ComponentType<VfxgraphComponent>
{
	VfxgraphComponentType()
	{
		typeName = "VfxgraphComponent";
		
		in("path", &VfxgraphComponent::path);
	}
};

#endif
