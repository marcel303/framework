include engine/ShaderVS.txt

shader_out vec3 v_position;
shader_out vec3 v_normal;
shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();
	vec3 normal = unpackNormal().xyz;
	vec4 color = unpackColor();

	gl_Position = objectToProjection(position);

	v_position = objectToView(position).xyz;
	v_normal = objectToView3(normal);
	v_color = color;
}
