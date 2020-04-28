include engine/ShaderVS.txt

uniform float time;

shader_out vec2 v_texcoord0;
shader_out vec3 v_normal;
shader_out vec3 v_normalV;

void main()
{
	vec4 position = unpackPosition();

	position = objectToProjection(position);

#if 0
	position.x += cos((time + (+ position.x + position.y - position.z) * 10.0)) * 0.008;
	position.y += cos((time + (- position.x + position.y + position.z) * 10.0)) * 0.008;
	position.z += cos((time + (+ position.x - position.y + position.z) * 10.0)) * 0.008;
#endif

	vec2 texcoord = unpackTexcoord(0);
	
	gl_Position = position;
	
	v_texcoord0 = texcoord;

	v_normal = unpackNormal().xyz;

	v_normalV = objectToProjection(vec4(v_normal, 0.0)).xyz;
}
