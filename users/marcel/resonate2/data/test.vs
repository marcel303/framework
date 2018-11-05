include engine/ShaderVS.txt

shader_out float radians;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;
	
	radians = in_texcoord.x;
}
