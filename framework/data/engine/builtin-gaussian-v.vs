include engine/ShaderVS.txt

shader_out vec2 texcoord;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;
	
	texcoord = unpackTexcoord(0);
}
