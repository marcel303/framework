include engine/ShaderVS.txt
include CubeSides.txt

shader_out vec3 v_normal;
shader_out vec2 v_texcoord;
shader_out flat int v_textureIndex;

uniform float textureBaseSize;

void main()
{
	vec4 inPosition = unpackPosition();
	
	vec4 worldPosition = objectToWorld(inPosition);
	
	vec4 position = objectToProjection(inPosition);
	
	vec3 normal = objectToWorld(unpackNormal()).xyz;
	
	vec2 texcoord = unpackTexcoord(0);
	
	//
	
	int maxElemIndex = 0;
	float maxElemValue = normal.x;
	
	if (abs(normal.y) > abs(maxElemValue))
	{
		maxElemIndex = 1;
		maxElemValue = normal.y;
	}
	
	if (abs(normal.z) > abs(maxElemValue))
	{
		maxElemIndex = 2;
		maxElemValue = normal.z;
	}
	
	int textureIndex = maxElemIndex * 2 + ((maxElemValue < 0.0) ? 0 : 1);

	//textureIndex = 0;

	//vec3 p = inPosition.xyz;
	vec3 p = worldPosition.xyz;
	p /= textureBaseSize;
#if 0
	p -= vec3(0.5);
	vec3 xAxis = vec3(1, 0, 0);
	vec3 yAxis = vec3(0, 1, 0);
	vec3 zAxis = vec3(0, 0, 1);
	mat3 mat = mat3(xAxis, yAxis, zAxis);
	p = p * mat;
	p += vec3(0.5);
#elif 1
	//p -= vec3(0.5);
	int baseIndex = textureIndex * 3*3;
	vec3 xAxis = vec3(
		cubeSideInfo.transforms[baseIndex + 0],
		cubeSideInfo.transforms[baseIndex + 1],
		cubeSideInfo.transforms[baseIndex + 2]);
	vec3 yAxis = vec3(
		cubeSideInfo.transforms[baseIndex + 3],
		cubeSideInfo.transforms[baseIndex + 4],
		cubeSideInfo.transforms[baseIndex + 5]);
	vec3 zAxis = vec3(
		cubeSideInfo.transforms[baseIndex + 6],
		cubeSideInfo.transforms[baseIndex + 7],
		cubeSideInfo.transforms[baseIndex + 8]);
	mat3 mat = mat3(xAxis, yAxis, zAxis);
	p = mat * p;
	p += vec3(0.5);
#endif
	texcoord.x = p.x;
	texcoord.y = p.y;
	
	//
	
	gl_Position = position;
	
	v_normal = normal;
	v_texcoord = texcoord;
	v_textureIndex = textureIndex;
}
