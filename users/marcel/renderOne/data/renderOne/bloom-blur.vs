include engine/ShaderVS.txt

shader_out vec2 texcoord;

void main()
{
	vec4 position = unpackPosition();

	texcoord = unpackTexcoord(0);

	gl_Position = ModelViewProjectionMatrix * position;
}
