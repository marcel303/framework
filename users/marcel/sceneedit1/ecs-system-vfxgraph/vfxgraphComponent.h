#pragma once

#include "component.h"
#include <string>

//

#define ENABLE_RESOURCE_TESTS 1 // todo : remove and disable

#if ENABLE_RESOURCE_TESTS

#include "resource.h"

struct TextureResource : Resource<TextureResource> // todo : move elsewhere
{
	uint32_t texture;
};

#endif

//

class Surface;

struct VfxGraphInstance;
struct VfxGraphManager_Basic;

struct VfxgraphComponent : Component<VfxgraphComponent>
{
	std::string path;
	bool drawToSurface = true;
	Surface * surface = nullptr;
	int surfaceWidth = 256;
	int surfaceHeight = 256;
	
	VfxGraphInstance * instance = nullptr;
	
#if ENABLE_RESOURCE_TESTS
	TextureResource textureResource;
	
	std::vector<int> array;
#endif
	
	virtual ~VfxgraphComponent() override final;
	
	virtual bool init() override final;
	
	void tick(const float dt);
	
	virtual void propertyChanged(void * address) override final;
};

struct VfxgraphComponentMgr : ComponentMgr<VfxgraphComponent>
{
	VfxGraphManager_Basic * vfxGraphMgr = nullptr;
	
	VfxgraphComponentMgr();
	virtual ~VfxgraphComponentMgr() override final;
	
	virtual void tick(const float dt) override final;
};

extern VfxgraphComponentMgr g_vfxgraphComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct VfxgraphComponentType : ComponentType<VfxgraphComponent>
{
	VfxgraphComponentType()
		: ComponentType("VfxgraphComponent", &g_vfxgraphComponentMgr)
	{
		add("path", &VfxgraphComponent::path)
			.addFlag(new ComponentMemberFlag_EditorType_FilePath);
		add("drawToSurface", &VfxgraphComponent::drawToSurface);
		add("surfaceWidth", &VfxgraphComponent::surfaceWidth);
		add("surfaceHeight", &VfxgraphComponent::surfaceHeight);
		
	#if ENABLE_RESOURCE_TESTS
		add("array", &VfxgraphComponent::array);
	#endif
	}
};

#endif
