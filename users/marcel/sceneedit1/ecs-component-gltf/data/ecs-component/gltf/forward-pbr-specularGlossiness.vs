include engine/ShaderVS.txt

shader_out vec3 v_position_view;
shader_out vec3 v_normal_view;
shader_out vec4 v_color;
shader_out vec2 v_texcoord0;
shader_out vec2 v_texcoord1;

void main()
{
	vec4 position = unpackPosition();
	vec3 normal = unpackNormal().xyz;

	gl_Position = objectToProjection(position);

	v_position_view = objectToView(position).xyz;
	v_normal_view = objectToView3(normal);
	v_color = unpackColor();
	v_texcoord0 = unpackTexcoord(0);
	v_texcoord1 = unpackTexcoord(1);
}
