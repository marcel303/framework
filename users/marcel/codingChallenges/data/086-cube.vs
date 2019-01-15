include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec3 v_normal;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;

	v_color = unpackColor();
	v_normal = unpackNormal().xyz;
}
