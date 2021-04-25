include engine/ShaderVS.txt

shader_out vec3 v_position_object;
shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(position);

	v_position_object = position.xyz;
	v_color = unpackColor();
}
