include engine/ShaderVS.txt

shader_out vec2 v_texcoord;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = ModelViewProjectionMatrix * position;
	
	v_texcoord = unpackTexcoord(0);
}
