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
	typeDB.addStructured<AngleAxis>("AngleAxis")
		.add("angle", &AngleAxis::angle)
			.addFlag(new ComponentMemberFlag_EditorType_AngleDegrees)
		.add("axis", &AngleAxis::axis)
			.addFlag(new ComponentMemberFlag_EditorType_OrientationVector);
	
	//
	
	typeDB.add(std::type_index(typeid(TransformComponent)), new TransformComponentType());
	typeDB.addStructured<ParameterDefinition>("parameter")
		.add("type", &ParameterDefinition::type)
		.add("name", &ParameterDefinition::name)
		.add("defaultValue", &ParameterDefinition::defaultValue)
		.add("min", &ParameterDefinition::min)
		.add("max", &ParameterDefinition::max);
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
