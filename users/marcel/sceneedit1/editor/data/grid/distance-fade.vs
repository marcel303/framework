include engine/ShaderVS.txt

uniform float scale;

shader_out vec3 v_position_object;
shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();
	
	position.xyz *= scale;

	gl_Position = objectToProjection(position);

	v_position_object = position.xyz;
	v_color = unpackColor();
}
