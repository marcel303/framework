//include engine/ShaderVS.txt
include engine/ShaderSkinnedVS.txt

shader_out vec3 v_position;
shader_out vec3 v_normal;
shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();
	
	v_position = objectToView(position).xyz;
	v_normal = objectToView(unpackNormal()).xyz;
	v_color = unpackColor();

	gl_Position = objectToProjection(position);
}
