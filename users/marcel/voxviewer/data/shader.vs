include engine/ShaderVS.txt

shader_out vec3 v_normal;
shader_out vec4 v_color;

void main()
{
	v_normal = objectToView(unpackNormal()).xyz;
	v_color = unpackColor();

	gl_Position = objectToProjection(unpackPosition());
}
