include engine/ShaderVS.txt

shader_out vec2 texcoord;
shader_out vec4 color;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;
	
	texcoord = vec2(in_texcoord);
	color = unpackColor();
}
