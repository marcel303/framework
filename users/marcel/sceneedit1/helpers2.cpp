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
#include "components/vfxgraphComponent.h"
#include "scene/sceneNodeComponent.h"

// todo : remove component mgr globals
CameraComponentMgr s_cameraComponentMgr;
GltfComponentMgr s_gltfComponentMgr;
LightComponentMgr s_lightComponentMgr;
ModelComponentMgr s_modelComponentMgr;
ParameterComponentMgr s_parameterComponentMgr;
RotateTransformComponentMgr s_rotateTransformComponentMgr;
SceneNodeComponentMgr s_sceneNodeComponentMgr;
TransformComponentMgr s_transformComponentMgr;
//VfxgraphComponentMgr s_vfxgraphComponentMgr;

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

void registerComponentTypes(TypeDB & typeDB)
{
	registerComponentType(new CameraComponentType(), &s_cameraComponentMgr);
	registerComponentType(new GltfComponentType(), &s_gltfComponentMgr);
	registerComponentType(new LightComponentType(), &s_lightComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	registerComponentType(new ParameterComponentType(), &s_parameterComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new SceneNodeComponentType(), &s_sceneNodeComponentMgr);
	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	//registerComponentType(new VfxgraphComponentType(), &s_vfxgraphComponentMgr);
}

//

#include "helpers.h"
#include "resource.h"

ResourceDatabase g_resourceDatabase;
