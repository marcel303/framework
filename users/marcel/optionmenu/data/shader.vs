include engine/ShaderVS.txt

shader_out vec4 v_position;
shader_out vec2 v_texcoord;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec2 texcoord = unpackTexcoord(0);
	
	gl_Position = position;
	
	v_position = position;
	v_texcoord = texcoord;
}
