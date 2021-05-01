#include "componentTypeDB.h"

#define DEFINE_COMPONENT_TYPES
#include "sceneNodeComponent.h"

ComponentTypeRegistration(SceneNodeComponentType);

void link_ecsscene() { }
