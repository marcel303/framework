#pragma once

#include "component.h"
#include "Mat4x4.h"

struct SceneNodeComponent : Component<SceneNodeComponent>
{
	Mat4x4 objectToWorld = Mat4x4(true);
};

struct SceneNodeComponentMgr : ComponentMgr<SceneNodeComponent>
{
};

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct SceneNodeComponentType : ComponentType<SceneNodeComponent>
{
	SceneNodeComponentType()
		: ComponentType("SceneNodeComponent")
	{
	}
};

#endif
