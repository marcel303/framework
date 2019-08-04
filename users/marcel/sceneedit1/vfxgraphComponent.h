#if 0

#pragma once

#include "component.h"
#include <string>

//

#include "resource.h"

struct TextureResource : Resource<TextureResource> // todo : move elsewhere
{
	uint32_t texture;
};

//

struct VfxGraphInstance;
struct VfxGraphManager_Basic;

struct VfxgraphComponent : Component<VfxgraphComponent>
{
	std::string path;
	
	VfxGraphInstance * instance = nullptr;
	
	TextureResource textureResource;
	
	std::vector<int> array;
	
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
		: ComponentType("VfxgraphComponent")
	{
		in("path", &VfxgraphComponent::path);
		
		//in("array", &VfxgraphComponent::array);
	}
};

#endif

#endif
