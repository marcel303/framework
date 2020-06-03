include engine/ShaderVS.txt

uniform float u_time;

shader_out vec2 v_texcoord;

void main()
{
	vec3 position = unpackPosition().xyz;

	vec3 oldPosition = position;
	position.x += sin(u_time * 0.023 * (1.0 + oldPosition.y + oldPosition.z));
	position.y += sin(u_time * 0.034 * (1.0 + oldPosition.z + oldPosition.x));
	position.z += sin(u_time * 0.045 * (1.0 + oldPosition.x + oldPosition.y));

	vec3 normal = normalize(position.xyz);
	vec3 tangent1 = normal.yzx;
	float distance = dot(normal, tangent1);
	//tangent1 += distance * normal;
	tangent1 -= distance * normal;
	tangent1 = normalize(tangent1);
	vec3 tangent2 = cross(tangent1, normal);

	float s = 0.004;
	tangent1 *= s;
	tangent2 *= s;

	int id = gl_VertexID % 6;
	if (id == 0) { position += - tangent1 - tangent2; v_texcoord = vec2(0, 0); }
	if (id == 1) { position +=   tangent1 - tangent2; v_texcoord = vec2(1, 0); }
	if (id == 2) { position += - tangent1 + tangent2; v_texcoord = vec2(0, 1); }
	if (id == 3) { position += - tangent1 + tangent2; v_texcoord = vec2(0, 1); }
	if (id == 4) { position +=   tangent1 - tangent2; v_texcoord = vec2(1, 0); }
	if (id == 5) { position +=   tangent1 + tangent2; v_texcoord = vec2(1, 1); }

	gl_Position = objectToProjection(vec4(position, 1.0));
}