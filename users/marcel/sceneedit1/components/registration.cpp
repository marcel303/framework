#include "componentTypeDB.h"

#define DEFINE_COMPONENT_TYPES
#include "cameraComponent.h"
#include "lightComponent.h"
#include "parameterComponent.h"
#include "rotateTransformComponent.h"
#include "transformComponent.h"

ComponentTypeRegistration(CameraComponentType);
ComponentTypeRegistration(LightComponentType);
ComponentTypeRegistration(ParameterComponentType);
ComponentTypeRegistration(RotateTransformComponentType);
ComponentTypeRegistration(TransformComponentType);

#include "sceneRenderRegistration.h"

void linkEcsComponents() { }
