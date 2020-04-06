include engine/ShaderVS.txt

shader_out vec3 v_position;
shader_out vec4 v_color;
shader_out vec3 v_normal;

void main()
{
	vec4 position = unpackPosition();
	vec4 color = unpackColor();
	vec4 normal = unpackNormal();
	
	gl_Position = objectToProjection(position);
	v_position = objectToView(position).xyz;
	v_color = color;
	v_normal = objectToView3(normal.xyz);
}
