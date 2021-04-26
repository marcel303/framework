#include "componentTypeDB.h"

#define DEFINE_COMPONENT_TYPES
#include "cameraComponent.h"
#include "gltfComponent.h"
#include "lightComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "rotateTransformComponent.h"
#include "transformComponent.h"

ComponentTypeRegistration(CameraComponentType);
ComponentTypeRegistration(GltfComponentType);
ComponentTypeRegistration(LightComponentType);
ComponentTypeRegistration(ModelComponentType);
ComponentTypeRegistration(ParameterComponentType);
ComponentTypeRegistration(RotateTransformComponentType);
ComponentTypeRegistration(TransformComponentType);

void linkEcsComponents() { }
