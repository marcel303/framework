#include "componentTypeDB.h"
#include "helpers.h"
#include "helpers2.h"
#include "reflection.h"

#define DEFINE_COMPONENT_TYPES
#include "components/cameraComponent.h"
#include "components/gltfComponent.h"
#include "components/lightComponent.h"
#include "components/modelComponent.h"
#include "components/parameterComponent.h"
#include "components/rotateTransformComponent.h"
#include "components/transformComponent.h"
//#include "components/vfxgraphComponent.h"
#include "scene/sceneNodeComponent.h"

//

TypeDB g_typeDB;

//

void registerBuiltinTypes(TypeDB & typeDB)
{
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("vec3", kDataType_Float3);
	typeDB.addPlain<Vec4>("vec4", kDataType_Float4);
	typeDB.addPlain<std::string>("string", kDataType_String);
	
// todo : move AngleAxis to ecs-component?
	AngleAxis::reflect(typeDB);
	
	//
	
// todo : why is transform component added here?
	typeDB.add(std::type_index(typeid(TransformComponent)), new TransformComponentType());
	
// todo : move ParameterDefinition reflection to ParameterComponentType?
	ParameterDefinition::reflect(typeDB);
}

void registerComponentTypes(TypeDB & typeDB, ComponentTypeDB & componentTypeDB)
{
	componentTypeDB.registerComponentType(new CameraComponentType());
	componentTypeDB.registerComponentType(new GltfComponentType());
	componentTypeDB.registerComponentType(new LightComponentType());
	componentTypeDB.registerComponentType(new ModelComponentType());
	componentTypeDB.registerComponentType(new ParameterComponentType());
	componentTypeDB.registerComponentType(new RotateTransformComponentType());
	componentTypeDB.registerComponentType(new SceneNodeComponentType());
	componentTypeDB.registerComponentType(new TransformComponentType());
	//componentTypeDB.registerComponentType(new VfxgraphComponentType());
}
