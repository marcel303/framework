include engine/ShaderVS.txt

uniform mat4x4 viewToWorld;

shader_out vec3 v_position;
shader_out vec3 v_normal;

void main()
{
	vec4 position = unpackPosition();
	vec3 normal = unpackNormal().xyz;

	gl_Position = objectToProjection(position);

	v_position = (viewToWorld * objectToView(position)).xyz;
	v_normal = normal;
}
