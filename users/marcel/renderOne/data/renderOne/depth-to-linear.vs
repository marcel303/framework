include engine/ShaderVS.txt

shader_out vec2 v_texcoord;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;
	
	v_texcoord = unpackTexcoord(0);
}
