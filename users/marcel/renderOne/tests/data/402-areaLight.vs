include engine/ShaderVS.txt

uniform mat4x4 viewToWorld;

uniform float time;

shader_out vec3 v_position;
shader_out vec3 v_normal;

void main()
{
	vec4 position = unpackPosition();
	vec3 normal = unpackNormal().xyz;

	position.xyz += normal * ((sin(time) + 0.5) / 1.5) * 0.05;

	gl_Position = objectToProjection(position);

	v_position = objectToView(position).xyz;
	v_normal = objectToView3(normal);
}
