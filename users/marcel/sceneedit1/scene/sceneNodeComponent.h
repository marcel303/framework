#pragma once

#include "component.h"
#include "Mat4x4.h"
#include <string>

struct SceneNodeComponent : Component<SceneNodeComponent>
{
	Mat4x4 objectToWorld = Mat4x4(true);
	
	std::string name;
	
	std::string attachedFromScene;
};

struct SceneNodeComponentMgr : ComponentMgr<SceneNodeComponent>
{
};

extern SceneNodeComponentMgr g_sceneNodeComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct SceneNodeComponentType : ComponentType<SceneNodeComponent>
{
	SceneNodeComponentType()
		: ComponentType("SceneNodeComponent", &g_sceneNodeComponentMgr)
	{
		add("name", &SceneNodeComponent::name);
		
		add("attachedFromScene", &SceneNodeComponent::attachedFromScene);
	}
};

#endif
