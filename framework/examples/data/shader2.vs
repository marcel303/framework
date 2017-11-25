include engine/ShaderVS.txt

shader_out vec2 v_texcoord0;

void main()
{
	vec4 position = unpackPosition();
	position = objectToProjection(position);
	
	vec2 texcoord = unpackTexcoord(0);
	
	//
	
	gl_Position = position;
	v_texcoord0 = texcoord;
}
